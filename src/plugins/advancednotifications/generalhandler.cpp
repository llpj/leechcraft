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

#include "generalhandler.h"
#include <interfaces/structures.h>
#include <interfaces/an/constants.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include "systemtrayhandler.h"
#include "visualhandler.h"
#include "audiohandler.h"
#include "cmdrunhandler.h"
#include "core.h"
#include "wmurgenthandler.h"
#include "rulesmanager.h"

namespace LeechCraft
{
namespace AdvancedNotifications
{
	GeneralHandler::GeneralHandler (ICoreProxy_ptr proxy)
	: Proxy_ (proxy)
	{
		Handlers_ << ConcreteHandlerBase_ptr (new SystemTrayHandler);
		Handlers_ << ConcreteHandlerBase_ptr (new VisualHandler);
		Handlers_ << ConcreteHandlerBase_ptr (new AudioHandler);
		Handlers_ << ConcreteHandlerBase_ptr (new CmdRunHandler);
		Handlers_ << ConcreteHandlerBase_ptr (new WMUrgentHandler);

		for (const auto& handler : Handlers_)
			handler->SetGeneralHandler (this);

		connect (Handlers_.first ().get (),
				SIGNAL (gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace)),
				this,
				SIGNAL (gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace)));

		Cat2IconName_ [AN::CatDownloads] = "folder-downloads";
		Cat2IconName_ [AN::CatIM] = "mail-unread-new";
		Cat2IconName_ [AN::CatOrganizer] = "view-calendar";
		Cat2IconName_ [AN::CatGeneric] = "preferences-desktop-notification-bell";
		Cat2IconName_ [AN::CatPackageManager] = "system-software-update";
		Cat2IconName_ [AN::CatMediaPlayer] = "applications-multimedia";
	}

	void GeneralHandler::Handle (const Entity& e)
	{
		if (e.Mime_ == "x-leechcraft/notification-rule-create")
		{
			Core::Instance ().GetRulesManager ()->HandleEntity (e);
			return;
		}

		if (e.Additional_ ["org.LC.AdvNotifications.EventCategory"] == "org.LC.AdvNotifications.Cancel")
		{
			for (const auto& handler : Handlers_)
				handler->Handle (e, NotificationRule ());
			return;
		}

		const auto& rules = Core::Instance ().GetRules (e);
		for (const auto& rule : rules)
		{
			const auto& methods = rule.GetMethods ();
			for (const auto& handler : Handlers_)
			{
				if (!(methods & handler->GetHandlerMethod ()))
					continue;

				handler->Handle (e, rule);
			}
		}
	}

	ICoreProxy_ptr GeneralHandler::GetProxy () const
	{
		return Proxy_;
	}

	QIcon GeneralHandler::GetIconForCategory (const QString& cat) const
	{
		const QString& name = Cat2IconName_.value (cat, "general");
		return Proxy_->GetIconThemeManager ()->GetIcon (name);
	}
}
}
