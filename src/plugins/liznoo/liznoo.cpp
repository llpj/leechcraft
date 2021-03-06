/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "liznoo.h"
#include <cmath>
#include <limits>
#include <QIcon>
#include <QAction>
#include <QMessageBox>
#include <QTimer>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/entitytesthandleresult.h>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/threads/futures.h>
#include <util/sll/delayedexecutor.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "batteryhistorydialog.h"
#include "quarkmanager.h"
#include "platform/screen/screenplatform.h"
#include "platform/battery/batteryplatform.h"

#if defined(Q_OS_LINUX)
	#include "platform/battery/upowerplatform.h"
	#include "platform/events/platformupowerlike.h"

	#ifdef USE_PMUTILS
		#include "platform/poweractions/pmutils.h"
	#else
		#include "platform/poweractions/upower.h"
	#endif

	#include "platform/screen/freedesktop.h"
	#include "platform/common/dbusthread.h"
	#include "platform/upower/upowerconnector.h"
	#include "platform/logind/logindconnector.h"
#elif defined(Q_OS_WIN32)
	#include "platform/battery/winapiplatform.h"
	#include "platform/events/platformwinapi.h"
	#include "platform/winapi/fakeqwidgetwinapi.h"
#elif defined(Q_OS_FREEBSD)
	#include "platform/battery/freebsdplatform.h"
	#include "platform/events/platformfreebsd.h"
	#include "platform/poweractions/freebsd.h"
	#include "platform/screen/freedesktop.h"
#elif defined(Q_OS_MAC)
	#include "platform/battery/macplatform.h"
	#include "platform/events/platformmac.h"
#else
	#pragma message ("Unsupported system")
#endif

namespace LeechCraft
{
namespace Liznoo
{
	const int HistSize = 300;
	const auto UpdateMsecs = 3000;

	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		qRegisterMetaType<BatteryInfo> ("Liznoo::BatteryInfo");

		Util::InstallTranslator ("liznoo");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (XmlSettingsManager::Instance (), "liznoosettings.xml");

#if defined(Q_OS_LINUX)
		const auto upowerThread = std::make_shared<DBusThread<UPower::UPowerConnector>> ();
		const auto logindThread = std::make_shared<DBusThread<Logind::LogindConnector>> ();

		PL_ = Events::MakeUPowerLike (upowerThread, Proxy_);
		PL_->SubscribeAvailable ([this, logindThread] (bool avail)
				{
					if (avail)
						return;

					qDebug () << Q_FUNC_INFO
							<< "UPower events backend is not available, trying logind...";
					Util::DelayDestruction (PL_);
					PL_ = Events::MakeUPowerLike (logindThread, Proxy_);
				});

		SPL_ = new Screen::Freedesktop (this);
		BatteryPlatform_ = std::make_shared<Battery::UPowerPlatform> (upowerThread);

	#ifdef USE_PMUTILS
		PowerActPlatform_ = std::make_shared<PowerActions::PMUtils> ();
	#else
		PowerActPlatform_ = std::make_shared<PowerActions::UPower> ();
	#endif

		upowerThread->start (QThread::IdlePriority);
		logindThread->start (QThread::IdlePriority);
#elif defined(Q_OS_WIN32)
		const auto widget = std::make_shared<WinAPI::FakeQWidgetWinAPI> ();

		PL_ = std::make_shared<Events::PlatformWinAPI> (widget, Proxy_);
		BatteryPlatform_ = std::make_shared<Battery::WinAPIPlatform> (widget);
#elif defined(Q_OS_FREEBSD)
		PL_ = std::make_shared<Events::PlatformFreeBSD> (Proxy_);
		PowerActPlatform_ = std::make_shared<PowerActions::FreeBSD> ();
		BatteryPlatform_ = std::make_shared<Battery::FreeBSDPlatform> ();
		SPL_ = new Screen::Freedesktop (this);
#elif defined(Q_OS_MAC)
		BatteryPlatform_ = std::make_shared<Battery::MacPlatform> ();
		PL_ = std::make_shared<Events::PlatformMac> (Proxy_);
#endif

		if (BatteryPlatform_)
			connect (BatteryPlatform_.get (),
					SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
					this,
					SLOT (handleBatteryInfo (Liznoo::BatteryInfo)));
		else
			qWarning () << Q_FUNC_INFO
					<< "battery backend is not available";

		const auto battTimer = new QTimer { this };
		connect (battTimer,
				SIGNAL (timeout ()),
				this,
				SLOT (handleUpdateHistory ()));
		battTimer->start (UpdateMsecs);

		Suspend_ = new QAction (tr ("Suspend"), this);
		connect (Suspend_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSuspendRequested ()));
		Suspend_->setProperty ("ActionIcon", "system-suspend");

		Hibernate_ = new QAction (tr ("Hibernate"), this);
		connect (Hibernate_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleHibernateRequested ()));
		Hibernate_->setProperty ("ActionIcon", "system-suspend-hibernate");

		connect (XSD_.get (),
				SIGNAL (pushButtonClicked (QString)),
				this,
				SLOT (handlePushButton (QString)));

		const auto qm = new QuarkManager;
		LiznooQuark_ = std::make_shared<QuarkComponent> ("liznoo", "LiznooQuark.qml");
		LiznooQuark_->DynamicProps_.append ({ "Liznoo_proxy", qm });

		connect (qm,
				SIGNAL (batteryHistoryDialogRequested (QString)),
				this,
				SLOT (handleHistoryTriggered (QString)));

		if (BatteryPlatform_)
			connect (BatteryPlatform_.get (),
					SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
					qm,
					SLOT (handleBatteryInfo (Liznoo::BatteryInfo)));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Liznoo";
	}

	void Plugin::Release ()
	{
		PL_.reset ();
		PowerActPlatform_.reset ();
		BatteryPlatform_.reset ();
	}

	QString Plugin::GetName () const
	{
		return "Liznoo";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("UPower/WinAPI-based power manager.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/liznoo/resources/images/liznoo.svg");
		return icon;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& entity) const
	{
		return entity.Mime_ == "x-leechcraft/power-management" ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity entity)
	{
		const auto& context = entity.Entity_.toString ();
		if (context == "ScreensaverProhibition")
		{
			if (!SPL_)
			{
				qWarning () << Q_FUNC_INFO
						<< "screen platform layer unavailable, screensaver prohibiton won't work";
				return;
			}

			const auto enable = entity.Additional_ ["Enable"].toBool ();
			const auto& id = entity.Additional_ ["ContextID"].toString ();

			SPL_->ProhibitScreensaver (enable, id);
		}
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace place) const
	{
#if QT_VERSION >= 0x050000
		Q_UNUSED (place);
		return {};
#else
		QList<QAction*> result;
		if (place == ActionsEmbedPlace::LCTray)
			result << Battery2Action_.values ();
		return result;
#endif
	}

	QMap<QString, QList<QAction*>> Plugin::GetMenuActions () const
	{
		QMap<QString, QList<QAction*>> result;
		result ["System"] << Suspend_;
		result ["System"] << Hibernate_;
		return result;
	}

	QuarkComponents_t Plugin::GetComponents () const
	{
#if QT_VERSION >= 0x050000
		return { LiznooQuark_ };
#else
		return {};
#endif
	}

#if QT_VERSION < 0x050000
	namespace
	{
		QString GetBattIconName (BatteryInfo info)
		{
			const bool isCharging = info.TimeToFull_ && !info.TimeToEmpty_;

			QString name = "battery-";
			if (isCharging)
				name += "charging-";

			if (info.Percentage_ < 15)
				name += "low";
			else if (info.Percentage_ < 30)
				name += "caution";
			else if (info.Percentage_ < 50)
				name += "040";
			else if (info.Percentage_ < 70)
				name += "060";
			else if (info.Percentage_ < 90)
				name += "080";
			else if (isCharging)
				name.chop (1);
			else
				name += "100";

			return name;
		}
	}
#endif

	void Plugin::CheckNotifications (const BatteryInfo& info)
	{
		auto check = [&info, this] (const std::function<bool (BatteryInfo)>& f)
		{
			if (!Battery2LastInfo_.contains (info.ID_))
				return f (info);

			return f (info) && !f (Battery2LastInfo_ [info.ID_]);
		};

		auto checkPerc = [] (const BatteryInfo& b, const QByteArray& prop)
		{
			if (!XmlSettingsManager::Instance ()->property ("NotifyOn" + prop).toBool ())
				return false;

			return b.Percentage_ <= XmlSettingsManager::Instance ()->
					property (prop + "Level").toInt ();
		};

		const bool isExtremeLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "ExtremeLowPower"); });
		const bool isLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "LowPower"); });

		const auto iem = Proxy_->GetEntityManager ();
		if (isExtremeLow || isLow)
			iem->HandleEntity (Util::MakeNotification ("Liznoo",
						tr ("Battery charge level is %1%.")
							.arg (static_cast<int> (info.Percentage_)),
						isLow ? PInfo_ : PWarning_));

		if (XmlSettingsManager::Instance ()->property ("NotifyOnPowerTransitions").toBool ())
		{
			const bool startedCharging = check ([] (const BatteryInfo& b)
					{ return b.TimeToFull_ && !b.TimeToEmpty_; });
			const bool startedDischarging = check ([] (const BatteryInfo& b)
					{ return !b.TimeToFull_ && b.TimeToEmpty_; });

			if (startedCharging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started charging."),
							PInfo_));
			else if (startedDischarging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started discharging."),
							PWarning_));
		}
	}

	void Plugin::ChangeState (PowerActions::Platform::State state)
	{
		if (!PowerActPlatform_)
			return;

		Util::Sequence (this, PowerActPlatform_->CanChangeState (state)) >>
				[state, this] (const PowerActions::Platform::QueryChangeStateResult& res)
				{
					if (res.CanChangeState_)
						PowerActPlatform_->ChangeState (state);
					else
					{
						const auto& msg = res.Reason_.isEmpty () ?
								tr ("Cannot change state.") :
								res.Reason_;
						const auto& entity = Util::MakeNotification ("Liznoo",
								msg, PCritical_);
						Proxy_->GetEntityManager ()->HandleEntity (entity);
					}
				};
	}

	void Plugin::handleBatteryInfo (BatteryInfo info)
	{
#if QT_VERSION < 0x050000
		const auto& iconName = GetBattIconName (info);
		if (!Battery2Action_.contains (info.ID_))
		{
			QAction *act = new QAction (tr ("Battery status"), this);
			act->setProperty ("WatchActionIconChange", true);
			act->setProperty ("Liznoo/BatteryID", info.ID_);

			act->setProperty ("Action/Class", GetUniqueID () + "/BatteryAction");
			act->setProperty ("Action/ID", GetUniqueID () + "/" + info.ID_);
			act->setProperty ("ActionIcon", iconName);

			emit gotActions ({ act }, ActionsEmbedPlace::LCTray);
			Battery2Action_ [info.ID_] = act;

			connect (act,
					SIGNAL (triggered ()),
					this,
					SLOT (handleHistoryTriggered ()));
		}
		else
			Battery2Action_ [info.ID_]->setProperty ("ActionIcon", iconName);
#endif

		CheckNotifications (info);

		Battery2LastInfo_ [info.ID_] = info;
	}

	void Plugin::handleUpdateHistory ()
	{
		for (const QString& id : Battery2LastInfo_.keys ())
		{
			auto& hist = Battery2History_ [id];
			hist << BatteryHistory (Battery2LastInfo_ [id]);
			if (hist.size () > HistSize)
				hist.removeFirst ();
		}

		for (const QString& id : Battery2Dialog_.keys ())
			Battery2Dialog_ [id]->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
	}

	void Plugin::handleHistoryTriggered ()
	{
		const auto& id = sender ()->property ("Liznoo/BatteryID").toString ();
		handleHistoryTriggered (id);
	}

	void Plugin::handleHistoryTriggered (const QString& id)
	{
		if (!Battery2History_.contains (id) ||
				Battery2Dialog_.contains (id))
		{
			if (auto dia = Battery2Dialog_.value (id))
				dia->close ();
			return;
		}

		auto dialog = new BatteryHistoryDialog (HistSize, UpdateMsecs / 1000.);
		dialog->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
		dialog->setAttribute (Qt::WA_DeleteOnClose);
		Battery2Dialog_ [id] = dialog;
		connect (dialog,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleBatteryDialogDestroyed ()));
		dialog->show ();
		dialog->activateWindow ();
		dialog->raise ();
	}

	void Plugin::handleBatteryDialogDestroyed ()
	{
		auto dia = static_cast<BatteryHistoryDialog*> (sender ());
		Battery2Dialog_.remove (Battery2Dialog_.key (dia));
	}

	void Plugin::handleSuspendRequested ()
	{
		ChangeState (PowerActions::Platform::State::Suspend);
	}

	void Plugin::handleHibernateRequested ()
	{
		ChangeState (PowerActions::Platform::State::Hibernate);
	}

	void Plugin::handlePushButton (const QString& button)
	{
		if (!PL_)
		{
			qWarning () << Q_FUNC_INFO
					<< "platform backend unavailable";

			QMessageBox::critical (nullptr,
					"LeechCraft",
					tr ("No platform backend is set, unable to send test power events."));

			return;
		}

		if (button == "TestSleep")
			PL_->emitGonnaSleep (1000);
		else if (button == "TestWake")
			PL_->emitWokeUp ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_liznoo, LeechCraft::Liznoo::Plugin);
