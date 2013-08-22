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

#include "updatesnotificationmanager.h"
#include <QTimer>
#include <util/xpc/util.h>
#include <util/models/modeliterator.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/an/constants.h>
#include "packagesmodel.h"
#include "core.h"

namespace LeechCraft
{
namespace LackMan
{
	UpdatesNotificationManager::UpdatesNotificationManager (PackagesModel *model,
			ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, PM_ (model)
	, Proxy_ (proxy)
	, NotifyScheduled_ (false)
	{
		connect (model,
				SIGNAL (dataChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleDataChanged (QModelIndex, QModelIndex)));
		if (const auto rc = model->rowCount ())
			handleDataChanged (model->index (0, 0), model->index (rc - 1, 0));
	}

	void UpdatesNotificationManager::ScheduleNotify ()
	{
		if (NotifyScheduled_)
			return;

		NotifyScheduled_ = true;
		QTimer::singleShot (5000,
				this,
				SLOT (notify ()));
	}

	void UpdatesNotificationManager::handleDataChanged (const QModelIndex& from, const QModelIndex& to)
	{
		bool changed = false;
		for (int i = from.row (); i <= to.row (); ++i)
		{
			const auto& idx = PM_->index (i, 0);
			const auto id = idx.data (PackagesModel::PMRPackageID).toInt ();
			const auto upgradable = idx.data (PackagesModel::PMRUpgradable).toBool ();

			if (UpgradablePackages_.contains (id) && !upgradable)
			{
				UpgradablePackages_.remove (id);
				changed = true;
			}
			else if (!UpgradablePackages_.contains (id) && upgradable)
			{
				UpgradablePackages_ << id;
				changed = true;
			}
		}

		if (changed)
			ScheduleNotify ();
	}

	void UpdatesNotificationManager::notify ()
	{
		NotifyScheduled_ = false;

		const auto upgradableCount = UpgradablePackages_.size ();
		QString bodyText;
		if (upgradableCount <= 3)
		{
			QStringList names;
			for (auto id : UpgradablePackages_)
				names << Core::Instance ().GetListPackageInfo (id).Name_;
			names.sort ();

			if (upgradableCount == 1)
				bodyText = tr ("A new version of %1 is available.")
						.arg ("<em>" + names.value (0) + "</em>");
			else
			{
				const auto& lastName = names.takeLast ();
				bodyText = tr ("New versions of %1 and %2 are available.")
						.arg ("<em>" + names.join ("</em>, <em>") + "</em>")
						.arg ("<em>" + lastName + "</em>");
			}
		}
		else
			bodyText = tr ("New versions are available for %n package(s).",
					nullptr, upgradableCount);

		const auto& entity = Util::MakeAN ("Lackman",
				bodyText,
				PInfo_,
				"org.LeechCraft.LackMan",
				AN::CatPackageManager,
				AN::TypePackageUpdated,
				"org.LeechCraft.LackMan",
				{ "Lackman" },
				0,
				upgradableCount);
		Proxy_->GetEntityManager ()->HandleEntity (entity);
	}
}
}
