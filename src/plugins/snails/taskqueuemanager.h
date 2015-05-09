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

#include <functional>
#include <boost/optional.hpp>
#include <QObject>
#include <QString>
#include <QMutex>
#include <QFutureInterface>
#include "valuedmetaargument.h"

namespace LeechCraft
{
namespace Snails
{
	struct TaskQueueItem
	{
		enum class Priority
		{
			Lowest = -50,
			Low = -10,
			Normal = 0,
			High = 10,
			Highest = 50
		};

		int Priority_ = static_cast<int> (Priority::Normal);

		QByteArray Method_;
		QList<ValuedMetaArgument> Args_;

		QByteArray ID_;

		std::shared_ptr<QFutureInterface<void>> Promise_ = std::make_shared<QFutureInterface<void>> ();

		TaskQueueItem ();
		TaskQueueItem (const QByteArray&,
				const QList<ValuedMetaArgument>&, const QByteArray& = {});
		TaskQueueItem (int priority, const QByteArray&,
				const QList<ValuedMetaArgument>&, const QByteArray& = {});
		TaskQueueItem (Priority priority, const QByteArray&,
				const QList<ValuedMetaArgument>&, const QByteArray& = {});
	};

	bool operator== (const TaskQueueItem&, const TaskQueueItem&);

	class AccountThreadWorker;

	class TaskQueueManager : public QObject
	{
		Q_OBJECT

		AccountThreadWorker * const ATW_;

		mutable QMutex ItemsMutex_;
		QList<TaskQueueItem> Items_;
	public:
		struct MergeResult
		{
			QList<int> ToRemove_;
			QList<QPair<int, QList<ValuedMetaArgument>>> ToUpdate_;
		};
		using MaybeMergeResult = boost::optional<MergeResult>;
		using TaskMerger = std::function<MaybeMergeResult (QList<TaskQueueItem>, TaskQueueItem)>;
	private:
		static QList<TaskMerger> Mergers_;
	public:
		TaskQueueManager (AccountThreadWorker*);

		static void RegisterTaskMerger (const TaskMerger&);

		void AddTasks (QList<TaskQueueItem>);
		bool HasItems () const;
		TaskQueueItem PopItem ();
	private:
		void Merge (MergeResult);

		template<typename Ex>
		bool HandleReconnect (const TaskQueueItem&, const Ex& ex, int recLevel);

		void HandleItem (const TaskQueueItem&, int recLevel = 0);
	private slots:
		void rotateTaskQueue ();
	signals:
		void gotTask ();
	};
}
}

