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

#pragma once

#include <QObject>
#include <QHash>
#include <interfaces/media/iradiostation.h>
#include <util/sll/oldcppkludges.h>

class QAbstractItemModel;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QTimer;
class QUrl;

namespace Media
{
	class IRadioStationProvider;
	struct AudioInfo;
}

namespace LeechCraft
{
namespace Util
{
	class MergeModel;
}

namespace LMP
{
	class Player;

	class RadioManager : public QObject
	{
		Q_OBJECT

		Util::MergeModel * const MergeModel_;
		QHash<const QAbstractItemModel*, Media::IRadioStationProvider*> Model2Prov_;

		QTimer *AutoRefreshTimer_;
	public:
		RadioManager (QObject* = 0);

		void InitProviders ();

		QAbstractItemModel* GetModel () const;

		void Refresh (const QModelIndex&);
		void AddUrl (const QModelIndex&, const QUrl&, const QString&);
		void RemoveUrl (const QModelIndex&);
		void Handle (const QModelIndex&, Player*);

		void HandleWokeUp ();

		QList<Media::AudioInfo> GetSources (const QModelIndex&) const;
		QList<Media::AudioInfo> GetSources (const QList<QModelIndex>&) const;

		Media::IRadioStation_ptr GetRadioStation (const QString&) const;
	private:
		void InitProvider (QObject*);

		template<typename F>
		Util::ResultOf_t<F (Media::IRadioStationProvider*, QModelIndex)>
			WithSourceProv (const QModelIndex&, F) const;

		template<typename Succ, typename Fail>
		Util::ResultOf_t<Succ (Media::IRadioStationProvider*, QModelIndex)>
			WithSourceProv (const QModelIndex&, Succ, Fail) const;
	public slots:
		void refreshAll ();
	private slots:
		void handleRefreshSettingsChanged ();
	};
}
}
