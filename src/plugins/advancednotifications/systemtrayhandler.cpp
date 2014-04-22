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

#include "systemtrayhandler.h"
#include <interfaces/structures.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include <QMenu>
#include <QPainter>
#include <QApplication>
#include <util/util.h>
#include <util/gui/util.h>
#include <util/gui/unhoverdeletemixin.h>
#include "generalhandler.h"
#include "xmlsettingsmanager.h"
#include "core.h"

#ifdef HAVE_QML
#include "qml/visualnotificationsview.h"
#endif

namespace LeechCraft
{
namespace AdvancedNotifications
{
	SystemTrayHandler::SystemTrayHandler ()
	{
	}

	SystemTrayHandler::~SystemTrayHandler ()
	{
		const auto& icons = Category2Icon_.values ();
		for (auto icon : icons)
		{
			icon->hide ();
			delete icon;
		}
	}

	NotificationMethod SystemTrayHandler::GetHandlerMethod () const
	{
		return NMTray;
	}

	namespace
	{
		QPixmap GetPixmap (const Entity& e, ICoreProxy_ptr proxy)
		{
			QPixmap pixmap = e.Additional_ ["NotificationPixmap"].value<QPixmap> ();
			if (pixmap.isNull ())
			{
				QString mi = "information";
				switch (e.Additional_ ["Priority"].toInt ())
				{
					case PWarning_:
						mi = "warning";
						break;
					case PCritical_:
						mi = "error";
					default:
						break;
				}

				pixmap = proxy->GetIconThemeManager ()->
						GetIcon ("dialog-" + mi).pixmap (QSize (64, 64));
			}
			return pixmap;
		}
	}

	void SystemTrayHandler::Handle (const Entity& e, const NotificationRule&)
	{
		const QString& cat = e.Additional_ ["org.LC.AdvNotifications.EventCategory"].toString ();
		const QString& eventId = e.Additional_ ["org.LC.AdvNotifications.EventID"].toString ();

		if (cat != "org.LC.AdvNotifications.Cancel")
		{
			PrepareSysTrayIcon (cat);
			PrepareLCTrayAction (cat);
			if (!Events_.contains (eventId))
			{
				EventData data;
				data.EventID_ = eventId;
				data.Count_ = 0;
				data.Category_ = cat;
				data.VisualPath_ = e.Additional_ ["org.LC.AdvNotifications.VisualPath"].toStringList ();
				data.HandlingObject_ = e.Additional_ ["HandlingObject"].value<QObject_ptr> ();
				data.Actions_ = e.Additional_ ["NotificationActions"].toStringList ();
				Events_ [eventId] = data;
			}

			const int delta = e.Additional_.value ("org.LC.AdvNotifications.DeltaCount", 0).toInt ();
			if (delta)
				Events_ [eventId].Count_ += delta;
			else
				Events_ [eventId].Count_ = e.Additional_.value ("org.LC.AdvNotifications.Count", 1).toInt ();
			Events_ [eventId].ExtendedText_ = e.Additional_ ["org.LC.AdvNotifications.ExtendedText"].toString ();
			Events_ [eventId].FullText_ = e.Additional_ ["org.LC.AdvNotifications.FullText"].toString ();

			Events_ [eventId].Pixmap_ = GetPixmap (e, GH_->GetProxy ());
		}
		else if (!Events_.remove (eventId))
			return;

		RebuildState ();
	}

	void SystemTrayHandler::PrepareSysTrayIcon (const QString& category)
	{
#ifdef Q_OS_MAC
		return;
#endif

		if (Category2Icon_.contains (category))
			return;

		QSystemTrayIcon *trayIcon = new QSystemTrayIcon (GH_->GetIconForCategory (category));
		trayIcon->setContextMenu (new QMenu ());
		Category2Icon_ [category] = trayIcon;

		connect (trayIcon,
				SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
				this,
				SLOT (handleTrayActivated (QSystemTrayIcon::ActivationReason)));

#ifdef HAVE_QML
		VisualNotificationsView *vnv = new VisualNotificationsView;
		connect (vnv,
				SIGNAL (actionTriggered (const QString&, int)),
				this,
				SLOT (handleActionTriggered (const QString&, int)));
		connect (vnv,
				SIGNAL (dismissEvent (const QString&)),
				this,
				SLOT (dismissNotification (const QString&)));
		Icon2NotificationView_ [trayIcon] = vnv;

		if (XmlSettingsManager::Instance ().property ("HideOnHoverOut").toBool ())
			new Util::UnhoverDeleteMixin (vnv, SLOT (hide ()));
#endif
	}

	void SystemTrayHandler::PrepareLCTrayAction (const QString& category)
	{
		if (Category2Action_.contains (category))
			return;

		QAction *action = new QAction (GH_->GetIconForCategory (category), category, this);
		action->setMenu (new QMenu ());
		Category2Action_ [category] = action;

		connect (action,
				SIGNAL (triggered ()),
				this,
				SLOT (handleLCAction ()));

		emit gotActions (QList<QAction*> () << action, ActionsEmbedPlace::LCTray);

#ifdef HAVE_QML
		VisualNotificationsView *vnv = new VisualNotificationsView;
		connect (vnv,
				SIGNAL (actionTriggered (const QString&, int)),
				this,
				SLOT (handleActionTriggered (const QString&, int)));
		connect (vnv,
				SIGNAL (dismissEvent (const QString&)),
				this,
				SLOT (dismissNotification (const QString&)));
		Action2NotificationView_ [action] = vnv;

		if (XmlSettingsManager::Instance ().property ("HideOnHoverOut").toBool ())
			new Util::UnhoverDeleteMixin (vnv, SLOT (hide ()));
#endif
	}

	void SystemTrayHandler::UpdateMenu (QMenu *menu, const QString& event, const EventData& data)
	{
		Q_FOREACH (const QString& pathItem, data.VisualPath_)
			menu = menu->addMenu (pathItem);

		if (!data.Pixmap_.isNull ())
			menu->setIcon (data.Pixmap_);
		menu->setToolTip (data.ExtendedText_);

		int actionIdx = 0;
		Q_FOREACH (const QString& actionName, data.Actions_)
		{
			QAction *action = menu->addAction (actionName);
			action->setProperty ("Index", actionIdx++);
			action->setProperty ("EventID", event);
			connect (action,
					SIGNAL (triggered ()),
					this,
					SLOT (handleActionTriggered ()), Qt::QueuedConnection);
		}

		QAction *dismiss = menu->addAction (tr ("Dismiss"));
		dismiss->setProperty ("EventID", event);
		connect (dismiss,
				SIGNAL (triggered ()),
				this,
				SLOT (dismissNotification ()), Qt::QueuedConnection);

		menu->addSeparator ();
		menu->addAction (data.ExtendedText_)->setEnabled (false);
	}

	void SystemTrayHandler::RebuildState ()
	{
		auto icons2hide = Category2Icon_.values ().toSet ();
		Q_FOREACH (QSystemTrayIcon *icon, icons2hide)
			icon->contextMenu ()->clear ();

		auto actsDel = Category2Action_.values ().toSet ();
		Q_FOREACH (QAction *action, actsDel)
			action->menu ()->clear ();

#ifdef HAVE_QML
		EventsForIcon_.clear ();
		EventsForAction_.clear ();
#endif

		QSet<QSystemTrayIcon*> visibleIcons;
		QSet<QAction*> actsUpd;

		int eventCount = 0;
		for (const auto& event : Events_.keys ())
		{
			const EventData& data = Events_ [event];

			QSystemTrayIcon *icon = Category2Icon_.value (data.Category_);
			if (icon)
			{
				icons2hide.remove (icon);
				visibleIcons << icon;
			}

			QAction *action = Category2Action_ [data.Category_];
			actsDel.remove (action);
			actsUpd << action;

#ifdef HAVE_QML
			if (icon)
				EventsForIcon_ [icon] << data;
			EventsForAction_ [action] << data;
#endif

			if (icon)
				UpdateMenu (icon->contextMenu (), event, data);
			UpdateMenu (action->menu (), event, data);

			eventCount += data.Count_;
		}

		const auto& entity = Util::MakeEntity (eventCount,
				QString (),
				LeechCraft::Internal,
				"x-leechcraft/notification-event-count-info");
		Core::Instance ().SendEntity (entity);

#ifdef HAVE_QML
		Q_FOREACH (QSystemTrayIcon *icon, Category2Icon_.values ())
		{
			VisualNotificationsView *view = Icon2NotificationView_ [icon];
			if (!view->isVisible ())
				continue;

			const auto& events = EventsForIcon_ [icon];
			view->SetEvents (events);
			if (events.isEmpty ())
				view->hide ();
		}

		Q_FOREACH (QAction *action, Category2Action_.values ())
		{
			VisualNotificationsView *view = Action2NotificationView_ [action];
			if (!view->isVisible ())
				continue;

			const auto& events = EventsForAction_ [action];
			view->SetEvents (events);
			if (events.isEmpty ())
				view->hide ();
		}
#endif

		Q_FOREACH (QSystemTrayIcon *icon, visibleIcons)
		{
			if (!icon->isVisible ())
				icon->show ();
			UpdateSysTrayIcon (icon);
		}

		Q_FOREACH (QAction *action, actsUpd)
			UpdateTrayAction (action);

		Q_FOREACH (QSystemTrayIcon *icon, icons2hide)
			icon->hide ();

		Q_FOREACH (QAction *action, actsDel)
		{
			Category2Action_.remove (Category2Action_.key (action));
			EventsForAction_.remove (action);
			delete Action2NotificationView_.take (action);
		}
		qDeleteAll (actsDel);
	}

	template<typename T>
	void SystemTrayHandler::UpdateIcon (T iconable, const QString& category)
	{
		QIcon icon = GH_->GetIconForCategory (category);
		if (!XmlSettingsManager::Instance ()
				.property ("EnableCounter." + category.toLatin1 ()).toBool ())
		{
			iconable->setIcon (icon);
			return;
		}

		int eventCount = 0;
		for (const auto& event : Events_)
			if (event.Category_ == category)
				eventCount += event.Count_;

		const auto& palette = qApp->palette ();

		QFont font = qApp->font ();
		font.setItalic (true);

		QIcon withText;
		for (const auto& size : icon.availableSizes ())
		{
			const auto& px = icon.pixmap (size);
			const auto& overlaid = Util::DrawOverlayText (px,
					QString::number (eventCount), font,
					QPen (palette.color (QPalette::ButtonText)),
					QBrush (palette.color (QPalette::Button)));
			withText.addPixmap (overlaid);
		}

		iconable->setIcon (withText);
	}

	void SystemTrayHandler::UpdateSysTrayIcon (QSystemTrayIcon *trayIcon)
	{
		UpdateIcon<QSystemTrayIcon*> (trayIcon, Category2Icon_.key (trayIcon));
	}

	void SystemTrayHandler::UpdateTrayAction (QAction *action)
	{
		UpdateIcon<QAction*> (action, Category2Action_.key (action));
	}

	void SystemTrayHandler::handleActionTriggered ()
	{
		const QString& event = sender ()->property ("EventID").toString ();
		const int index = sender ()->property ("Index").toInt ();

		handleActionTriggered (event, index);
	}

	void SystemTrayHandler::handleActionTriggered (const QString& event, int index)
	{
		if (!Events_.contains (event))
		{
			qWarning () << Q_FUNC_INFO
					<< "no such event"
					<< event;
			return;
		}

		QMetaObject::invokeMethod (Events_ [event].HandlingObject_.get (),
				"notificationActionTriggered",
				Qt::QueuedConnection,
				Q_ARG (int, index));
	}

	void SystemTrayHandler::dismissNotification ()
	{
		dismissNotification (sender ()->property ("EventID").toString ());
	}

	void SystemTrayHandler::dismissNotification (const QString& event)
	{
		if (Events_.remove (event))
			RebuildState ();
	}

	namespace
	{
		void ShowVNV (VisualNotificationsView *view, const QList<EventData>& events)
		{
			if (!view->isVisible ())
			{
				view->SetEvents (events);
				view->move (Util::FitRectScreen (QCursor::pos (), view->size ()));
			}
			view->setVisible (!view->isVisible ());
		}
	}

	void SystemTrayHandler::handleTrayActivated (QSystemTrayIcon::ActivationReason reason)
	{
		if (reason != QSystemTrayIcon::Trigger)
			return;

#ifdef HAVE_QML
		QSystemTrayIcon *trayIcon = qobject_cast<QSystemTrayIcon*> (sender ());
		if (!trayIcon)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QSystemTrayIcon";
			return;
		}

		ShowVNV (Icon2NotificationView_ [trayIcon], EventsForIcon_ [trayIcon]);
#endif
	}

	void SystemTrayHandler::handleLCAction ()
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QSystemTrayIcon";
			return;
		}

#ifndef HAVE_QML
		action->menu ()->show ();
#else
		ShowVNV (Action2NotificationView_ [action], EventsForAction_ [action]);
#endif
	}
}
}
