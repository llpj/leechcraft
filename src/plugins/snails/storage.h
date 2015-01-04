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
#include <QDir>
#include <QSettings>
#include <QHash>
#include <QSet>
#include "message.h"

namespace LeechCraft
{
namespace Snails
{
	class Account;

	class AccountDatabase;
	typedef std::shared_ptr<AccountDatabase> AccountDatabase_ptr;

	class Storage : public QObject
	{
		Q_OBJECT

		QDir SDir_;
		QSettings Settings_;
		QHash<QByteArray, bool> IsMessageRead_;

		QHash<Account*, AccountDatabase_ptr> AccountBases_;
		QHash<Account*, QHash<QByteArray, Message_ptr>> PendingSaveMessages_;

		QHash<QObject*, Account*> FutureWatcher2Account_;
	public:
		Storage (QObject* = 0);

		void SaveMessages (Account*, const QStringList& folders, const QList<Message_ptr>&);

		MessageSet LoadMessages (Account*);
		Message_ptr LoadMessage (Account*, const QStringList& folder, const QByteArray& id);
		QList<Message_ptr> LoadMessages (Account*, const QStringList& folder, const QList<QByteArray>& ids);

		QList<QByteArray> LoadIDs (Account*, const QStringList& folder);
		void RemoveMessage (Account*, const QStringList&, const QByteArray&);

		int GetNumMessages (Account*) const;
		int GetNumMessages (Account*, const QStringList& folder);
		int GetNumUnread (Account*, const QStringList& folder);
		bool HasMessagesIn (Account*) const;

		bool IsMessageRead (Account*, const QStringList& folder, const QByteArray&);
	private:
		void RemoveMessageFile (Account*, const QStringList&, const QByteArray&);
	private:
		QDir DirForAccount (Account*) const;
		AccountDatabase_ptr BaseForAccount (Account*);

		void AddMessage (Message_ptr, Account*);
		void UpdateCaches (Message_ptr);
	private slots:
		void handleMessagesSaved ();
	};
}
}
