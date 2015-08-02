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

#include "previewhandler.h"
#include <util/xpc/util.h>
#include <interfaces/media/iaudiopile.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/ientitymanager.h>
#include "player.h"
#include "core.h"
#include "previewcharacteristicinfo.h"

namespace LeechCraft
{
namespace LMP
{
	PreviewHandler::PreviewHandler (Player *player, QObject *parent)
	: QObject (parent)
	, Player_ (player)
	{
		Providers_ = Core::Instance ().GetProxy ()->
				GetPluginsManager ()->GetAllCastableTo<Media::IAudioPile*> ();
	}

	void PreviewHandler::HandlePending (Media::IPendingAudioSearch *pending)
	{
		connect (pending->GetQObject (),
				SIGNAL (ready ()),
				this,
				SLOT (handlePendingReady ()));
	}

	void PreviewHandler::previewArtist (const QString& artist)
	{
		Media::AudioSearchRequest req;
		req.Artist_ = artist;
		RequestPreview (req);
	}

	void PreviewHandler::previewTrack (const QString& track, const QString& artist)
	{
		Media::AudioSearchRequest req;
		req.Title_ = track;
		req.Artist_ = artist;
		RequestPreview (req);
	}

	void PreviewHandler::previewTrack (const QString& track, const QString& artist, int length)
	{
		Media::AudioSearchRequest req;
		req.Title_ = track;
		req.Artist_ = artist;
		req.TrackLength_ = length;
		RequestPreview (req);
	}

	void PreviewHandler::previewAlbum (const QString& artist, const QString& album, const QList<QPair<QString, int>>& tracks)
	{
		Media::AudioSearchRequest req;
		req.Artist_ = artist;

		auto& storedAlbumTracks = Artist2Album2Tracks_ [artist] [album];

		for (const auto& pair : tracks)
		{
			req.Title_ = pair.first;
			req.TrackLength_ = pair.second;

			PendingTrackInfo info =
			{
				artist,
				album,
				req.Title_
			};

			const auto& pendings = RequestPreview (req);
			for (const auto& pending : pendings)
				Pending2Track_ [pending] = info;

			storedAlbumTracks [req.Title_] += pendings.size ();
		}
	}

	QList<Media::IPendingAudioSearch*> PreviewHandler::RequestPreview (const Media::AudioSearchRequest& req)
	{
		QList<Media::IPendingAudioSearch*> pendings;
		for (auto prov : Providers_)
		{
			auto pending = prov->Search (req);
			HandlePending (pending);
			pendings << pending;
		}
		return pendings;
	}

	/** Checks whether the given album is fully completed or not.
	 */
	void PreviewHandler::CheckPendingAlbum (Media::IPendingAudioSearch *pending)
	{
		if (!Pending2Track_.contains (pending))
			return;

		const auto& info = Pending2Track_.take (pending);

		auto& tracks = Artist2Album2Tracks_ [info.Artist_] [info.Album_];

		/** If we don't have info.Track_ in our pending list it was fulfilled
		 * by another audio pile and we don't have to do anything.
		 */
		if (!tracks.contains (info.Track_))
			return;

		/** If this pending object has any results then it fulfills this track
		 * request, and we remove the track from the pending list. Otherwise
		 * we reduce the amount of available pending requests by one.
		 */
		if (!pending->GetResults ().isEmpty ())
			tracks.remove (info.Track_);
		else
			--tracks [info.Track_];

		/** Then we check if all interesting pending requests have finished.
		 * That is, if we have at least one track with non-zero pending count,
		 * or `tracks` is empty, we should either wait further or don't have to
		 * do anything. In case tracklist is empty we should clear
		 * Artist2Album2Tracks_.
		 */
		if (std::find_if (tracks.begin (), tracks.end (),
				[] (int c) { return c > 0; }) != tracks.end ())
			return;

		if (tracks.isEmpty ())
		{
			auto& artist = Artist2Album2Tracks_ [info.Artist_];
			artist.remove (info.Album_);
			if (artist.isEmpty ())
				Artist2Album2Tracks_.remove (info.Artist_);
			return;
		}

		/** Not all tracks were fulfilled and all pending requests have
		 * finished. We have to upset the user now :(
		 */
		const auto& e = Util::MakeNotification ("LMP",
				tr ("Not all tracks were fetched for album %1 by %2: %n track(s) weren't found.",
						0, tracks.size ())
					.arg (info.Album_)
					.arg (info.Artist_),
				PWarning_);
		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);
	}

	void PreviewHandler::handlePendingReady ()
	{
		auto pending = qobject_cast<Media::IPendingAudioSearch*> (sender ());

		QList<AudioSource> sources;
		QSet<QUrl> urls;
		QSet<PreviewCharacteristicInfo> infos;
		for (const auto& res : pending->GetResults ())
		{
			if (urls.contains (res.Source_))
				continue;
			urls.insert (res.Source_);

			const PreviewCharacteristicInfo checkInfo { res.Info_ };
			if (infos.contains (checkInfo))
				continue;
			infos << checkInfo;

			Player_->PrepareURLInfo (res.Source_, MediaInfo::FromAudioInfo (res.Info_));
			sources << AudioSource (res.Source_);
		}

		if (!sources.isEmpty ())
			Player_->Enqueue (sources, Player::EnqueueNone);

		CheckPendingAlbum (pending);
	}
}
}
