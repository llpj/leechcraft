/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "rosenthal.h"
#include <QIcon>
#include <QApplication>
#include <QTextEdit>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTranslator>
#include <util/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "highlighter.h"
#include "checker.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Rosenthal
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		Translator_.reset (Util::InstallTranslator ("azoth_rosenthal"));

		SettingsDialog_.reset (new Util::XmlSettingsDialog);
		SettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"azothrosenthalsettings.xml");

		connect (SettingsDialog_.get (),
				SIGNAL (pushButtonClicked (QString)),
				this,
				SLOT (handlePushButtonClicked (QString)));

		Checker_ = new Checker ();
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.Rosenthal";
	}

	void Plugin::Release ()
	{
		delete Checker_;
		Checker_ = nullptr;
	}

	QString Plugin::GetName () const
	{
		return "Azoth Rosenthal";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Provides spellchecking for Azoth.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/rosenthal/resources/images/rosenthal.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	bool Plugin::eventFilter (QObject *obj, QEvent *event)
	{
		QPoint eventPos;
		if (event->type () == QEvent::ContextMenu)
			eventPos = static_cast<QContextMenuEvent*> (event)->pos ();
		else if (event->type () == QEvent::MouseButtonPress)
		{
			QMouseEvent *me = static_cast<QMouseEvent*> (event);
			if (me->buttons () & Qt::RightButton)
				eventPos = me->pos ();
			else
				return QObject::eventFilter (obj, event);
		}
		else
			return QObject::eventFilter (obj, event);

		QTextEdit *edit = qobject_cast<QTextEdit*> (obj);
		const QPoint& curPos = edit->mapToGlobal (eventPos);

		QTextCursor cur = edit->cursorForPosition (eventPos);
		QString word = cur.block ().text ();
		const int pos = cur.columnNumber ();
		const int end = word.indexOf (QRegExp("\\W+"), pos);
		const int begin = word.lastIndexOf (QRegExp("\\W+"), pos);
		word = word.mid (begin + 1, end - begin - 1);

		QMenu *menu = edit->createStandardContextMenu (curPos);

		const auto& words = Checker_->GetPropositions (word);
		if (!words.isEmpty ())
		{
			QList<QAction*> acts;
			for (const auto& word : words)
			{
				QAction *act = new QAction (word, menu);
				acts << act;
				connect (act,
						SIGNAL (triggered ()),
						this,
						SLOT (handleCorrectionTriggered ()));
				act->setProperty ("TextEdit", QVariant::fromValue<QObject*> (edit));
				act->setProperty ("CursorPos", eventPos);
			}

			QAction *before = menu->actions ().first ();
			menu->insertActions (before, acts);
			menu->insertSeparator (before);
		}

		menu->exec (curPos);

		return true;
	}

	void Plugin::handlePushButtonClicked (const QString& name)
	{
		if (name != "InstallDicts")
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown button"
					<< name;
			return;
		}

		auto e = Util::MakeEntity ("ListPackages",
				QString (),
				FromUserInitiated,
				"x-leechcraft/package-manager-action");
		e.Additional_ ["Tags"] = QStringList ("dicts");

		Proxy_->GetEntityManager ()->HandleEntity (e);
	}

	void Plugin::hookChatTabCreated (LeechCraft::IHookProxy_ptr,
			QObject *chatTab, QObject*, QWebView*)
	{
		QTextEdit *edit = 0;
		QMetaObject::invokeMethod (chatTab,
				"getMsgEdit",
				Q_RETURN_ARG (QTextEdit*, edit));

		Highlighter *hl = new Highlighter (Checker_, edit->document ());
		Highlighters_ << hl;
		connect (hl,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleHighlighterDestroyed ()));

		edit->installEventFilter (this);
	}

	void Plugin::handleCorrectionTriggered ()
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
			return;

		QTextEdit *edit = qobject_cast<QTextEdit*> (action->property ("TextEdit").value<QObject*> ());
		const QPoint& pos = action->property ("CursorPos").toPoint ();
		QTextCursor cur = edit->cursorForPosition (pos);
		cur.select (QTextCursor::WordUnderCursor);
		cur.deleteChar ();
		cur.insertText (action->text ());
	}

	void Plugin::handleHighlighterDestroyed ()
	{
		Highlighters_.removeAll (static_cast<Highlighter*> (sender ()));
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_rosenthal, LeechCraft::Azoth::Rosenthal::Plugin);
