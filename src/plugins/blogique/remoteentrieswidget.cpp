/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "remoteentrieswidget.h"
#include <stdexcept>
#include <QStandardItemModel>
#include "entriesfilterproxymodel.h"
#include "xmlsettingsmanager.h"
#include "core.h"
#include "utils.h"

namespace LeechCraft
{
namespace Blogique
{
	RemoteEntriesWidget::RemoteEntriesWidget (QWidget *parent,
			Qt::WindowFlags f)
	: QWidget (parent, f)
	, Account_ (0)
	, RemoteEntriesModel_ (new QStandardItemModel (this))
	, FilterProxyModel_ (new EntriesFilterProxyModel (this))
	{
		Ui_.setupUi (this);

		if (!Ui_.RemoteEntriesCalendarSplitter_->
				restoreState (XmlSettingsManager::Instance ()
					.property ("LocalEntriesCalendarSplitterPosition")
					.toByteArray ()))
			Ui_.RemoteEntriesCalendarSplitter_->setStretchFactor (1, 4);

		connect (Ui_.RemoteEntriesCalendarSplitter_,
				SIGNAL (splitterMoved (int, int)),
				this,
				SLOT (saveSplitterPosition (int, int)));

		connect (Ui_.RemoteEntriesCalendar_,
				SIGNAL (activated (QDate)),
				this,
				SLOT (loadPostsByDate (QDate)));

		QAction *openRemoteEntryInNewTab = new QAction (tr ("Open in new tab"), this);
		QAction *openRemoteEntryInCurrentTab = new QAction (tr ("Open here"), this);

		RemoteEntriesModel_->setHorizontalHeaderLabels ({ tr ("Date"), tr ("Name") });
		FilterProxyModel_->setSourceModel (RemoteEntriesModel_);
		Ui_.RemoteEntriesView_->setModel (FilterProxyModel_);
		connect (openRemoteEntryInCurrentTab,
				SIGNAL (triggered ()),
				this,
				SLOT (handleOpenRemoteEntryInCurrentTab ()));
		connect (openRemoteEntryInNewTab,
				SIGNAL (triggered ()),
				this,
				SLOT (handleOpenRemoteEntryInNewTab ()));
		Ui_.RemoteEntriesView_->setContextMenuPolicy (Qt::ActionsContextMenu);
		Ui_.RemoteEntriesView_->addActions ({ openRemoteEntryInNewTab,
				openRemoteEntryInCurrentTab });
	}

	QString RemoteEntriesWidget::GetName () const
	{
		return tr ("Remote entries");
	}

	void RemoteEntriesWidget::SetAccount (IAccount *account)
	{
		for (auto act : LoadActions_)
			Ui_.LoadRemoteEntries_->removeAction (act);
		LoadActions_.clear ();

		Account_ = account;

		auto actions = account->GetUpdateActions ();
		Ui_.LoadRemoteEntries_->addActions (actions);
		LoadActions_ = actions;
	}

	Entry RemoteEntriesWidget::LoadFullEntry (qint64 id)
	{
		if (!Account_)
			return Entry ();

		try
		{
			return Core::Instance ().GetStorage ()->
					GetEntry (Account_->GetAccountID (), id);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error fetching full local entry"
					<< e.what ();
			return Entry ();
		}
	}

	void RemoteEntriesWidget::FillView (const QList<Entry>& entries)
	{
		RemoteEntriesModel_->removeRows (0, RemoteEntriesModel_->rowCount());
		for (const auto& entry : entries)
		{
			const auto& items = Utils::CreateEntriesViewRow (entry);
			if (items.isEmpty ())
				continue;

			RemoteEntriesModel_->appendRow (items);
			Item2Entry_ [items.first ()] = entry;
		}
		Ui_.RemoteEntriesView_->resizeColumnToContents (0);
	}

	void RemoteEntriesWidget::FillStatistic ()
	{
		if (!Account_)
			return;

		QMap<QDate, int> statistic;
		try
		{
			statistic = Core::Instance ().GetStorage ()
					->GetEntriesCountByDate (Account_->GetAccountID ());
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
			<< "error fetching entries"
			<< e.what ();
		}

		Ui_.RemoteEntriesCalendar_->SetStatistic (statistic);
	}

	void RemoteEntriesWidget::clear ()
	{
		FillView (QList<Entry> ());
	}

	void RemoteEntriesWidget::saveSplitterPosition (int pos, int index)
	{
		XmlSettingsManager::Instance ()
				.setProperty ("RemoteEntriesCalendarSplitterPosition",
						Ui_.RemoteEntriesCalendarSplitter_->saveState ());
	}

	void RemoteEntriesWidget::loadPostsByDate (const QDate& date)
	{
		if (!Account_)
			return;

		QList<Entry> entries;
		try
		{
			entries = Core::Instance ().GetStorage ()->
					GetEntriesByDate (Account_->GetAccountID (), date);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error fetching entries"
					<< e.what ();
		}

		FillView (entries);
	}

	void RemoteEntriesWidget::handleOpenRemoteEntryInCurrentTab (const QModelIndex& index)
	{
		QModelIndex idx = index.isValid () ?
			index :
			Ui_.RemoteEntriesView_->currentIndex ();
		if (!idx.isValid ())
			return;

		idx = idx.sibling (idx.row (), Utils::EntriesViewColumns::Date);
		const Entry& e = LoadFullEntry (idx.data (Utils::EntryIdRole::DBIdRole)
				.toLongLong ());

		emit fillCurrentWidgetWithRemoteEntry (e);
	}

	void RemoteEntriesWidget::handleOpenRemoteEntryInNewTab (const QModelIndex& index)
	{
		QModelIndex idx = index.isValid () ?
			index :
			Ui_.RemoteEntriesView_->currentIndex ();
		if (!idx.isValid ())
			return;

		if (!Account_)
			return;

		idx = idx.sibling (idx.row (), Utils::EntriesViewColumns::Date);
		const Entry& e = LoadFullEntry (idx.data (Utils::EntryIdRole::DBIdRole)
				.toLongLong ());

		emit fillNewWidgetWithRemoteEntry (e, Account_->GetAccountID ());
	}

	void RemoteEntriesWidget::on_RemoteEntriesFilter__textChanged (const QString& text)
	{
		FilterProxyModel_->setFilterFixedString (text);
	}
}
}