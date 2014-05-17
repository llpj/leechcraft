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

#include "torrenttab.h"
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QSortFilterProxyModel>
#include <util/tags/tagscompleter.h>
#include <util/gui/clearlineeditaddon.h>
#include <util/gui/lineeditbuttonmanager.h>
#include <util/xpc/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/itagsmanager.h>
#include "core.h"
#include "addtorrent.h"
#include "addmultipletorrents.h"
#include "ipfilterdialog.h"
#include "newtorrentwizard.h"
#include "trackerschanger.h"
#include "movetorrentfiles.h"
#include "tabviewproxymodel.h"

namespace LeechCraft
{
namespace Plugins
{
namespace BitTorrent
{
	TorrentTab::TorrentTab (const TabClassInfo& tc, QObject *mt)
	: TC_ (tc)
	, ParentMT_ (mt)
	, Toolbar_ (new QToolBar ("BitTorrent"))
	, ViewFilter_ (new TabViewProxyModel (this))
	{
		Ui_.setupUi (this);

		ViewFilter_->setDynamicSortFilter (true);
		ViewFilter_->setSourceModel (Core::Instance ());

		Ui_.TorrentsView_->setModel (ViewFilter_);
		connect (Ui_.TorrentsView_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleTorrentSelected (QModelIndex)));
		Ui_.TorrentsView_->sortByColumn (Core::ColumnID, Qt::SortOrder::AscendingOrder);

		const auto& fm = Ui_.TorrentsView_->fontMetrics ();
		QHeaderView *header = Ui_.TorrentsView_->header ();
		header->resizeSection (Core::Columns::ColumnID, fm.width ("999"));
		header->resizeSection (Core::Columns::ColumnName, fm.width ("boardwalk.empire.s03e02.hdtv.720p.ac3.rus.eng.novafilm.tv.mkv") * 1.3);

		auto buttonMgr = new Util::LineEditButtonManager (Ui_.SearchLine_);
		new Util::TagsCompleter (Ui_.SearchLine_);
		Ui_.SearchLine_->AddSelector (buttonMgr);
		new Util::ClearLineEditAddon (Core::Instance ()->GetProxy (), Ui_.SearchLine_, buttonMgr);
		connect (Ui_.SearchLine_,
				SIGNAL (textChanged (QString)),
				ViewFilter_,
				SLOT (setFilterFixedString (QString)));

		connect (Ui_.TorrentStateFilter_,
				SIGNAL (currentIndexChanged (int)),
				ViewFilter_,
				SLOT (setStateFilterMode (int)));

		OpenTorrent_ = new QAction (tr ("Open torrent..."), Toolbar_);
		OpenTorrent_->setShortcut (Qt::Key_Insert);
		OpenTorrent_->setProperty ("ActionIcon", "document-open");
		connect (OpenTorrent_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleOpenTorrentTriggered ()));

		CreateTorrent_ = new QAction (tr ("Create torrent..."), Toolbar_);
		CreateTorrent_->setProperty ("ActionIcon", "document-new");
		connect (CreateTorrent_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleCreateTorrentTriggered ()));

		OpenMultipleTorrents_ = new QAction (tr ("Open multiple torrents..."), Toolbar_);
		OpenMultipleTorrents_->setProperty ("ActionIcon", "document-open-folder");
		connect (OpenMultipleTorrents_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleOpenMultipleTorrentsTriggered ()));

		IPFilter_ = new QAction (tr ("IP filter..."), Toolbar_);
		IPFilter_->setProperty ("ActionIcon", "view-filter");
		connect (IPFilter_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleIPFilterTriggered ()));

		RemoveTorrent_ = new QAction (tr ("Remove"), Toolbar_);
		RemoveTorrent_->setShortcut (tr ("Del"));
		RemoveTorrent_->setProperty ("ActionIcon", "list-remove");
		connect (RemoveTorrent_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleRemoveTorrentTriggered ()));

		Resume_ = new QAction (tr ("Resume"), Toolbar_);
		Resume_->setShortcut (tr ("R"));
		Resume_->setProperty ("ActionIcon", "media-playback-start");
		connect (Resume_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleResumeTriggered ()));

		Stop_ = new QAction (tr ("Pause"), Toolbar_);
		Stop_->setShortcut (tr ("S"));
		Stop_->setProperty ("ActionIcon", "media-playback-pause");
		connect (Stop_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleStopTriggered ()));

		MoveUp_ = new QAction (tr ("Move up"), Toolbar_);
		MoveUp_->setShortcut (Qt::CTRL + Qt::Key_Up);
		MoveUp_->setProperty ("ActionIcon", "go-up");
		connect (MoveUp_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMoveUpTriggered ()));

		MoveDown_ = new QAction (tr ("Move down"), Toolbar_);
		MoveDown_->setShortcut (Qt::CTRL + Qt::Key_Down);
		MoveDown_->setProperty ("ActionIcon", "go-down");
		connect (MoveDown_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMoveDownTriggered ()));

		MoveToTop_ = new QAction (tr ("Move to top"), Toolbar_);
		MoveToTop_->setShortcut (Qt::CTRL + Qt::SHIFT + Qt::Key_Up);
		MoveToTop_->setProperty ("ActionIcon", "go-top");
		connect (MoveToTop_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMoveToTopTriggered ()));

		MoveToBottom_ = new QAction (tr ("Move to bottom"), Toolbar_);
		MoveToBottom_->setShortcut (Qt::CTRL + Qt::SHIFT + Qt::Key_Down);
		MoveToBottom_->setProperty ("ActionIcon", "go-bottom");
		connect (MoveToBottom_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMoveToBottomTriggered ()));

		ForceReannounce_ = new QAction (tr ("Reannounce"), Toolbar_);
		ForceReannounce_->setShortcut (tr ("F"));
		ForceReannounce_->setProperty ("ActionIcon", "network-wireless");
		connect (ForceReannounce_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleForceReannounceTriggered ()));

		ForceRecheck_ = new QAction (tr ("Recheck"), Toolbar_);
		ForceRecheck_->setProperty ("ActionIcon", "tools-check-spelling");
		connect (ForceRecheck_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleForceRecheckTriggered ()));

		MoveFiles_ = new QAction (tr ("Move files..."), Toolbar_);
		MoveFiles_->setShortcut (tr ("M"));
		MoveFiles_->setProperty ("ActionIcon", "transform-move");
		connect (MoveFiles_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMoveFilesTriggered ()));

		ChangeTrackers_ = new QAction (tr ("Change trackers..."), Toolbar_);
		ChangeTrackers_->setShortcut (tr ("C"));
		ChangeTrackers_->setProperty ("ActionIcon", "view-media-playlist");
		connect (ChangeTrackers_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleChangeTrackersTriggered ()));
		Ui_.Tabs_->SetChangeTrackersAction (ChangeTrackers_);

		MakeMagnetLink_ = new QAction (tr ("Make magnet link..."), Toolbar_);
		MakeMagnetLink_->setProperty ("ActionIcon", "insert-link");
		connect (MakeMagnetLink_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMakeMagnetLinkTriggered ()));

		Toolbar_->addAction (OpenTorrent_);
		Toolbar_->addAction (RemoveTorrent_);
		Toolbar_->addSeparator ();
		Toolbar_->addAction (Resume_);
		Toolbar_->addAction (Stop_);
		Toolbar_->addSeparator ();
		Toolbar_->addAction (MoveUp_);
		Toolbar_->addAction (MoveDown_);
		Toolbar_->addAction (MoveToTop_);
		Toolbar_->addAction (MoveToBottom_);
		Toolbar_->addSeparator ();
		Toolbar_->addAction (ForceReannounce_);
		Toolbar_->addAction (ForceRecheck_);
		Toolbar_->addAction (MoveFiles_);
		Toolbar_->addAction (ChangeTrackers_);
		Toolbar_->addAction (MakeMagnetLink_);

		setActionsEnabled ();
		connect (Ui_.TorrentsView_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (setActionsEnabled ()));
		/*
		Toolbar_->addSeparator ();
		DownSelectorAction_ = new SpeedSelectorAction ("Down", this);
		DownSelectorAction_->handleSpeedsChanged ();
		Toolbar_->addAction (DownSelectorAction_);
		UpSelectorAction_ = new SpeedSelectorAction ("Up", this);
		UpSelectorAction_->handleSpeedsChanged ();
		Toolbar_->addAction (UpSelectorAction_);

		connect (DownSelectorAction_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (handleFastSpeedComboboxes ()));
		connect (UpSelectorAction_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (handleFastSpeedComboboxes ()));
				*/
	}

	TabClassInfo TorrentTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* TorrentTab::ParentMultiTabs ()
	{
		return ParentMT_;
	}

	void TorrentTab::Remove ()
	{
		emit removeTab (this);
	}

	QToolBar* TorrentTab::GetToolBar () const
	{
		return Toolbar_;
	}

	int TorrentTab::GetCurrentTorrent () const
	{
		return ViewFilter_->mapToSource (Ui_.TorrentsView_->currentIndex ()).row ();
	}

	QList<int> TorrentTab::GetSelectedRows () const
	{
		QList<int> result;
		Q_FOREACH (const auto& idx, GetSelectedRowIndexes ())
			result << idx.row ();
		return result;
	}

	QModelIndexList TorrentTab::GetSelectedRowIndexes () const
	{
		QModelIndexList result;
		Q_FOREACH (const auto& idx, Ui_.TorrentsView_->selectionModel ()->selectedRows ())
			result << ViewFilter_->mapToSource (idx);
		return result;
	}

	void TorrentTab::handleTorrentSelected (const QModelIndex& index)
	{
		Ui_.Tabs_->SetCurrentIndex (ViewFilter_->mapToSource (index).row ());
	}

	void TorrentTab::setActionsEnabled ()
	{
		const auto& actions
		{
			Resume_, Stop_, MakeMagnetLink_, RemoveTorrent_,
			MoveUp_, MoveDown_, MoveToTop_, MoveToBottom_,
			ForceReannounce_, ForceRecheck_, MoveFiles_, ChangeTrackers_
		};
		const bool enable = Ui_.TorrentsView_->currentIndex ().isValid ();

		for (auto action : actions)
			action->setEnabled (enable);
	}

	void TorrentTab::on_TorrentsView__customContextMenuRequested (const QPoint& point)
	{
		QMenu menu;
		menu.addActions ({ Resume_, Stop_, MakeMagnetLink_, RemoveTorrent_ });
		menu.addSeparator ();
		menu.addActions ({ MoveToTop_, MoveUp_, MoveDown_, MoveToBottom_ });
		menu.addSeparator ();
		menu.addActions ({ ForceReannounce_, ForceRecheck_, MoveFiles_, ChangeTrackers_ });
		menu.exec (Ui_.TorrentsView_->viewport ()->mapToGlobal (point));
	}

	void TorrentTab::handleOpenTorrentTriggered ()
	{
		AddTorrent dia;
		if (dia.exec () == QDialog::Rejected)
			return;

		const auto& filename = dia.GetFilename ();
		const auto& path = dia.GetSavePath ();
		bool tryLive = dia.GetTryLive ();
		const auto& files = dia.GetSelectedFiles ();
		const auto& tags = dia.GetTags ();

		TaskParameters tp = FromUserInitiated;
		if (dia.GetAddType () != Core::Started)
			tp |= NoAutostart;
		Core::Instance ()->AddFile (filename,
				path,
				tags,
				tryLive,
				files,
				tp);

		setActionsEnabled ();
	}

	void TorrentTab::handleOpenMultipleTorrentsTriggered ()
	{
		AddMultipleTorrents dialog;
		std::unique_ptr<Util::TagsCompleter> completer (new Util::TagsCompleter (dialog.GetEdit (), this));
		dialog.GetEdit ()->AddSelector ();

		if (dialog.exec () == QDialog::Rejected)
			return;

		TaskParameters tp = FromUserInitiated;
		if (dialog.GetAddType () != Core::Started)
			tp |= NoAutostart;

		QString savePath = dialog.GetSaveDirectory (),
				openPath = dialog.GetOpenDirectory ();
		QDir dir (openPath);
		QStringList names = dir.entryList (QStringList ("*.torrent"));
		QStringList tags = dialog.GetTags ();
		for (int i = 0; i < names.size (); ++i)
		{
			QString name = openPath;
			if (!name.endsWith ('/'))
				name += '/';
			name += names.at (i);
			Core::Instance ()->AddFile (name, savePath, tags, false);
		}
		setActionsEnabled ();
	}

	void TorrentTab::handleIPFilterTriggered ()
	{
		IPFilterDialog dia;
		if (dia.exec () != QDialog::Accepted)
			return;

		Core::Instance ()->ClearFilter ();
		const auto& filter = dia.GetFilter ();
		Q_FOREACH (const auto& pair, filter)
			Core::Instance ()->BanPeers (pair.first, pair.second);
	}

	void TorrentTab::handleCreateTorrentTriggered ()
	{
		NewTorrentWizard wizard;
		if (wizard.exec () == QDialog::Accepted)
			Core::Instance ()->MakeTorrent (wizard.GetParams ());
		setActionsEnabled ();
	}

	void TorrentTab::handleRemoveTorrentTriggered ()
	{
		auto rows = GetSelectedRows ();

		QMessageBox confirm (QMessageBox::Question,
				"LeechCraft BitTorrent",
				tr ("Do you really want to delete %n torrent(s)?", 0, rows.size ()),
				QMessageBox::Cancel);
		auto deleteTorrentsButton = confirm.addButton (tr ("&Delete"),
				QMessageBox::ActionRole);
		auto deleteTorrentsAndFilesButton = confirm.addButton (tr ("Delete with &files"),
				QMessageBox::ActionRole);
		confirm.setDefaultButton (QMessageBox::Cancel);

		confirm.exec ();

		int roptions = libtorrent::session::none;
		if (confirm.clickedButton () == deleteTorrentsAndFilesButton)
			roptions |= libtorrent::session::delete_files;
		else if (confirm.clickedButton () == deleteTorrentsButton)
			;// do nothing
		else
			return;

		std::sort (rows.begin (), rows.end (), std::greater<int> ());

		Q_FOREACH (int row, rows)
			Core::Instance ()->RemoveTorrent (row, roptions);
		Ui_.Tabs_->InvalidateSelection ();
		setActionsEnabled ();
	}

	void TorrentTab::handleResumeTriggered ()
	{
		Q_FOREACH (int row, GetSelectedRows ())
			Core::Instance ()->ResumeTorrent (row);
		setActionsEnabled ();
	}

	void TorrentTab::handleStopTriggered ()
	{
		Q_FOREACH (int row, GetSelectedRows ())
			Core::Instance ()->PauseTorrent (row);
		setActionsEnabled ();
	}

	namespace
	{
		template<typename T>
		std::vector<T> List2Vector (const QList<T>& list)
		{
			std::vector<T> result;
			result.reserve (list.size ());
			std::copy (list.begin (), list.end (), std::back_inserter (result));
			return result;
		}
	}

	void TorrentTab::handleMoveUpTriggered ()
	{
		const auto& sis = GetSelectedRowIndexes ();
		const auto& selections = GetSelectedRows ();

		Core::Instance ()->MoveUp (List2Vector (selections));

		auto sel = Ui_.TorrentsView_->selectionModel ();
		sel->clearSelection ();

		QItemSelection selection;
		for (const auto& si : sis)
		{
			auto sibling = si.sibling (std::max (si.row () - 1, 0), si.column ());
			selection.select (sibling, sibling);
		}

		sel->select (selection, QItemSelectionModel::Rows | QItemSelectionModel::Select);
	}

	void TorrentTab::handleMoveDownTriggered ()
	{
		const auto& sis = GetSelectedRowIndexes ();
		const auto& selections = GetSelectedRows ();

		Core::Instance ()->MoveDown (List2Vector (selections));

		auto sel = Ui_.TorrentsView_->selectionModel ();
		sel->clearSelection ();

		QItemSelection selection;
		const auto& rowCount = Core::Instance ()->rowCount ();
		for (const auto& si : sis)
		{
			auto sibling = si.sibling (std::min (si.row () + 1, rowCount - 1), 0);
			selection.select (sibling, sibling);
		}

		sel->select (selection, QItemSelectionModel::Rows | QItemSelectionModel::Select);
	}

	void TorrentTab::handleMoveToTopTriggered ()
	{
		try
		{
			Core::Instance ()->MoveToTop (List2Vector (GetSelectedRows ()));
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< e.what ();
			return;
		}
	}

	void TorrentTab::handleMoveToBottomTriggered ()
	{
		try
		{
			Core::Instance ()->MoveToBottom (List2Vector (GetSelectedRows ()));
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< e.what ();
			return;
		}
	}

	void TorrentTab::handleForceReannounceTriggered ()
	{
		try
		{
			Q_FOREACH (int torrent, GetSelectedRows ())
				Core::Instance ()->ForceReannounce (torrent);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< e.what ();
			return;
		}
	}

	void TorrentTab::handleForceRecheckTriggered ()
	{
		try
		{
			Q_FOREACH (int torrent, GetSelectedRows ())
				Core::Instance ()->ForceRecheck (torrent);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< e.what ();
			return;
		}
	}

	void TorrentTab::handleChangeTrackersTriggered ()
	{
		const auto& sis = GetSelectedRowIndexes ();

		std::vector<libtorrent::announce_entry> allTrackers;
		Q_FOREACH (const auto& si, sis)
		{
			auto those = Core::Instance ()->GetTrackers (si.row ());
			std::copy (those.begin (), those.end (), std::back_inserter (allTrackers));
		}

		if (allTrackers.empty ())
			allTrackers = Core::Instance ()->
					GetTrackers (Core::Instance ()->
							GetCurrentTorrent ());

		std::stable_sort (allTrackers.begin (), allTrackers.end (),
				[] (const libtorrent::announce_entry& l, const libtorrent::announce_entry& r)
					{ return l.url < r.url; });

		auto newLast = std::unique (allTrackers.begin (), allTrackers.end (),
				[] (const libtorrent::announce_entry& l, const libtorrent::announce_entry& r)
					{ return l.url == r.url; });

		allTrackers.erase (newLast, allTrackers.end ());

		if (allTrackers.empty ())
			return;

		TrackersChanger changer;
		changer.SetTrackers (allTrackers);
		if (changer.exec () != QDialog::Accepted)
			return;

		const auto& trackers = changer.GetTrackers ();
		Q_FOREACH (const auto& si, sis)
			Core::Instance ()->SetTrackers (trackers, si.row ());
	}

	void TorrentTab::handleMoveFilesTriggered ()
	{
		const int current = GetCurrentTorrent ();
		QString oldDir = Core::Instance ()->GetTorrentDirectory (current);
		MoveTorrentFiles mtf (oldDir);
		if (mtf.exec () == QDialog::Rejected)
			return;
		QString newDir = mtf.GetNewLocation ();
		if (oldDir == newDir)
			return;

		if (!Core::Instance ()->MoveTorrentFiles (newDir, current))
		{
			QString text = tr ("Failed to move torrent's files from %1 to %2")
					.arg (oldDir)
					.arg (newDir);
			const auto& e = Util::MakeNotification ("BitTorrent", text, PCritical_);
			Core::Instance ()->GetProxy ()->GetEntityManager ()->HandleEntity (e);
		}
	}

	void TorrentTab::handleMakeMagnetLinkTriggered ()
	{
		QString magnet = Core::Instance ()->GetMagnetLink (GetCurrentTorrent ());
		if (magnet.isEmpty ())
			return;

		QInputDialog *dia = new QInputDialog ();
		dia->setWindowTitle ("LeechCraft");
		dia->setLabelText (tr ("Magnet link:"));
		dia->setAttribute (Qt::WA_DeleteOnClose);
		dia->setInputMode (QInputDialog::TextInput);
		dia->setTextValue (magnet);
		dia->resize (700, dia->height ());
		dia->show ();
	}
}
}
}
