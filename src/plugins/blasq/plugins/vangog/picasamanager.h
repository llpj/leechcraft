/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include <functional>
#include <QObject>
#include <QDateTime>
#include <QDomDocument>
#include <QStringList>
#include <QQueue>
#include <QUrl>
#include <QVariant>
#include "structures.h"

class QNetworkReply;

namespace LeechCraft
{
namespace Blasq
{
namespace Vangog
{
	class PicasaAccount;

	class PicasaManager : public QObject
	{
		Q_OBJECT

		PicasaAccount *Account_;
		QQueue<std::function<void (const QString&)>> ApiCallsQueue_;
		QString AccessToken_;
		QDateTime AccessTokenExpireDate_;
		bool FirstRequest_;

	public:
		PicasaManager (PicasaAccount *account, QObject *parent = 0);

		QString GetAccessToken () const;
		QDateTime GetAccessTokenExpireDate () const;

		void UpdateCollections ();
		void UpdatePhotos (const QByteArray& albumId);
	private:
		QByteArray CreateDomDocumentFromReply (QNetworkReply *reply, QDomDocument &document);
		void RequestAccessToken ();
		void ParseError (const QVariantMap& map);

		void RequestCollections (const QString& key);
		void RequestPhotos (const QByteArray& albumId, const QString& key);

		QList<Album> ParseAlbums (const QDomDocument& document);

	private slots:
		void handleAuthTokenRequestFinished ();
		void handleRequestCollectionFinished ();
		void handleRequestPhotosFinished ();

	signals:
		void gotAlbums (const QList<Album>& albums);
		void gotPhotos (const QList<Photo>& photos);
	};
}
}
}
