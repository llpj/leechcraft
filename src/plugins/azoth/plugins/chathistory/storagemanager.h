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

#include "storage.h"

namespace LeechCraft
{
namespace Azoth
{
namespace ChatHistory
{
	class StorageThread;
	class Core;

	class StorageManager : public QObject
	{
		StorageThread *StorageThread_;
		Core * const Core_;
	public:
		StorageManager (Core*);
		~StorageManager ();

		void Process (QObject*);
		void AddLogItems (const QString&, const QString&, const QString&, const QList<LogItem>&, bool);

		QFuture<IHistoryPlugin::MaxTimestampResult_t> GetMaxTimestamp (const QString&);

		QFuture<QStringList> GetOurAccounts ();

		QFuture<UsersForAccountResult_t> GetUsersForAccount (const QString&);

		QFuture<ChatLogsResult_t> GetChatLogs (const QString& accountId, const QString& entryId,
				int backpages, int amount);

		QFuture<SearchResult_t> Search (const QString& accountId, const QString& entryId,
				const QString& text, int shift, bool cs);
		QFuture<SearchResult_t> Search (const QString& accountId, const QString& entryId, const QDateTime& dt);

		QFuture<DaysResult_t> GetDaysForSheet (const QString& accountId, const QString& entryId, int year, int month);
		void ClearHistory (const QString& accountId, const QString& entryId);

		void RegenUsersCache ();
	private:
		void StartStorage ();
		void HandleStorageError (const Storage::InitializationError_t&);
		void HandleDumpFinished (qint64, qint64);
	};
}
}
}
