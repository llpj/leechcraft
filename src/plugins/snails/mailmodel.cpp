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

#include "mailmodel.h"
#include <util/util.h>

namespace LeechCraft
{
namespace Snails
{
	struct MailModel::TreeNode : std::enable_shared_from_this<TreeNode>
	{
		QByteArray FolderID_;

		QByteArray MsgID_;

		TreeNode_wptr Parent_;
		QList<TreeNode_ptr> Children_;

		int Row () const
		{
			const auto& parent = Parent_.lock ();
			if (!parent)
				return -1;

			const auto pos = std::find_if (parent->Children_.begin (), parent->Children_.end (),
					[this] (const TreeNode_ptr& other) { return other.get () == this; });
			if (pos == parent->Children_.end ())
			{
				qWarning () << Q_FUNC_INFO
						<< "unknown row for item"
						<< FolderID_;
				return -1;
			}

			return std::distance (parent->Children_.begin (), pos);
		}

		TreeNode () = default;

		TreeNode (const Message_ptr& msg, const TreeNode_ptr& parent)
		: FolderID_ { msg->GetFolderID () }
		, MsgID_ { msg->GetMessageID () }
		, Parent_ { parent }
		{
		}
	};

	MailModel::MailModel (QObject *parent)
	: QAbstractItemModel { parent }
	, Headers_ { tr ("From"), tr ("Subject"), tr ("Date"), tr ("Size")  }
	, Folder_ { "INBOX" }
	, Root_ { std::make_shared<TreeNode> () }
	{
	}

	QVariant MailModel::headerData (int section, Qt::Orientation orient, int role) const
	{
		if (orient != Qt::Horizontal || role != Qt::DisplayRole)
			return {};

		return Headers_.value (section);
	}

	int MailModel::columnCount (const QModelIndex&) const
	{
		return Headers_.size ();
	}

	QVariant MailModel::data (const QModelIndex& index, int role) const
	{
		const auto structItem = static_cast<TreeNode*> (index.internalPointer ());
		if (structItem == Root_.get ())
			return {};

		const auto& msg = GetMessageByFolderId (structItem->FolderID_);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< "no message for ID"
					<< structItem->FolderID_;
			return {};
		}

		switch (role)
		{
		case Qt::DisplayRole:
		case Sort:
			break;
		case ID:
			return msg->GetFolderID ();
		case ReadStatus:
			return msg->IsRead ();
		default:
			return {};
		}

		switch (static_cast<Column> (index.column ()))
		{
		case Column::From:
		{
			const auto& addr = msg->GetAddress (Message::Address::From);
			return addr.first.isEmpty () ? addr.second : addr.first;
		}
		case Column::Subject:
			return msg->GetSubject ();
		case Column::Date:
			if (role == Sort)
				return msg->GetDate ();
			else
				return msg->GetDate ().toString ();
		case Column::Size:
			if (role == Sort)
				return msg->GetSize ();
			else
				return Util::MakePrettySize (msg->GetSize ());
		default:
			return {};
		}
	}

	QModelIndex MailModel::index (int row, int column, const QModelIndex& parent) const
	{
		const auto structItem = parent.isValid () ?
				static_cast<TreeNode*> (parent.internalPointer ()) :
				Root_.get ();
		const auto childItem = structItem->Children_.value (row).get ();
		if (!childItem)
			return {};

		return createIndex (row, column, childItem);
	}

	QModelIndex MailModel::parent (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return {};

		const auto structItem = static_cast<TreeNode*> (index.internalPointer ());
		const auto& parentItem = structItem->Parent_.lock ();
		if (parentItem == Root_)
			return {};

		return createIndex (parentItem->Row (), 0, parentItem.get ());
	}

	int MailModel::rowCount (const QModelIndex& parent) const
	{
		const auto structItem = parent.isValid () ?
				static_cast<TreeNode*> (parent.internalPointer ()) :
				Root_.get ();
		return structItem->Children_.size ();
	}

	void MailModel::SetFolder (const QStringList& folder)
	{
		Folder_ = folder;
	}

	QStringList MailModel::GetCurrentFolder () const
	{
		return Folder_;
	}

	void MailModel::Clear ()
	{
		if (Messages_.isEmpty ())
			return;

		beginRemoveRows ({}, 0, Messages_.size () - 1);
		Messages_.clear ();
		Root_->Children_.clear ();
		FolderId2Nodes_.clear ();
		MsgId2FolderId_.clear ();
		endRemoveRows ();
	}

	void MailModel::Append (QList<Message_ptr> messages)
	{
		if (messages.isEmpty ())
			return;

		for (auto i = messages.begin (); i != messages.end (); )
		{
			const auto& msg = *i;

			if (!msg->GetFolders ().contains (Folder_))
			{
				i = messages.erase (i);
				continue;
			}

			if (Update (msg))
			{
				i = messages.erase (i);
				continue;
			}

			++i;
		}

		if (messages.isEmpty ())
			return;

		std::sort (messages.begin (), messages.end (),
				[] (const Message_ptr& left, const Message_ptr& right)
					{ return left->GetDate () < right->GetDate (); });

		Messages_ += messages;

		for (const auto& msg : messages)
		{
			const auto& msgId = msg->GetMessageID ();
			if (!msgId.isEmpty ())
				MsgId2FolderId_ [msgId] = msg->GetFolderID ();
		}

		for (const auto& msg : messages)
			if (!AppendStructured (msg))
			{
				const auto& node = std::make_shared<TreeNode> (msg, Root_);
				beginInsertRows ({}, Root_->Children_.size (), Root_->Children_.size ());
				Root_->Children_.append (node);
				FolderId2Nodes_ [msg->GetFolderID ()] << node;
				endInsertRows ();
			}
	}

	bool MailModel::Update (const Message_ptr& msg)
	{
		const auto pos = std::find_if (Messages_.begin (), Messages_.end (),
				[&msg] (const Message_ptr& other)
					{ return other->GetFolderID () == msg->GetFolderID (); });
		if (pos == Messages_.end ())
			return false;

		if (*pos != msg)
		{
			*pos = msg;
			for (const auto& indexPair : GetIndexes (msg->GetFolderID (), { 0, columnCount () - 1 }))
				emit dataChanged (indexPair.value (0), indexPair.value (1));
		}

		return true;
	}

	bool MailModel::Remove (const QByteArray& id)
	{
		const auto msgPos = std::find_if (Messages_.begin (), Messages_.end (),
				[&id] (const Message_ptr& other) { return other->GetFolderID () == id; });
		if (msgPos == Messages_.end ())
			return false;

		for (const auto& node : FolderId2Nodes_.value (id))
			RemoveNode (node);

		FolderId2Nodes_.remove (id);
		MsgId2FolderId_.remove ((*msgPos)->GetMessageID ());
		Messages_.erase (msgPos);

		return true;
	}

	void MailModel::RemoveNode (const TreeNode_ptr& node)
	{
		const auto& parent = node->Parent_.lock ();

		const auto& parentIndex = parent == Root_ ?
				QModelIndex {} :
				createIndex (parent->Row (), 0, parent.get ());

		const auto row = node->Row ();

		if (const auto childCount = node->Children_.size ())
		{
			const auto& nodeIndex = createIndex (row, 0, node.get ());

			beginRemoveRows (nodeIndex, 0, childCount - 1);
			auto childNodes = std::move (node->Children_);
			node->Children_.clear ();
			endRemoveRows ();

			for (const auto& childNode : childNodes)
				childNode->Parent_ = parent;

			beginInsertRows (parentIndex,
					parent->Children_.size (),
					parent->Children_.size () + childCount - 1);
			parent->Children_ += childNodes;
			endInsertRows ();
		}

		beginRemoveRows (parentIndex, row, row);
		parent->Children_.removeOne (node);
		endRemoveRows ();
	}

	bool MailModel::AppendStructured (const Message_ptr& msg)
	{
		auto refs = msg->GetReferences ();
		for (const auto& replyTo : msg->GetInReplyTo ())
			if (!refs.contains (replyTo))
				refs << replyTo;

		if (refs.isEmpty ())
			return false;

		const auto& replyTo = refs.last ();
		const auto& folderId = MsgId2FolderId_.value (replyTo);
		if (folderId.isEmpty ())
		{
			qDebug () << Q_FUNC_INFO
					<< folderId
					<< replyTo
					<< "not found";
			return false;
		}

		const auto& indexes = GetIndexes (folderId, 0);
		for (const auto& parentIndex : indexes)
		{
			const auto parentNode = static_cast<TreeNode*> (parentIndex.internalPointer ());
			const auto row = parentNode->Children_.size ();

			const auto& node = std::make_shared<TreeNode> (msg, parentNode->shared_from_this ());
			beginInsertRows (parentIndex, row, row);
			parentNode->Children_ << node;
			FolderId2Nodes_ [msg->GetFolderID ()] << node;
			endInsertRows ();
		}

		return !indexes.isEmpty ();
	}

	QList<QModelIndex> MailModel::GetIndexes (const QByteArray& folderId, int column) const
	{
		QList<QModelIndex> result;
		for (const auto& node : FolderId2Nodes_.value (folderId))
			result << createIndex (node->Row (), column, node.get ());
		return result;
	}

	QList<QList<QModelIndex>> MailModel::GetIndexes (const QByteArray& folderId, const QList<int>& columns) const
	{
		QList<QList<QModelIndex>> result;
		for (const auto& node : FolderId2Nodes_.value (folderId))
		{
			QList<QModelIndex> subresult;
			for (const auto column : columns)
				subresult << createIndex (node->Row (), column, node.get ());
			result << subresult;
		}
		return result;
	}

	Message_ptr MailModel::GetMessageByFolderId (const QByteArray& id) const
	{
		const auto pos = std::find_if (Messages_.begin (), Messages_.end (),
				[&id] (const Message_ptr& msg) { return msg->GetFolderID () == id; });
		if (pos == Messages_.end ())
			return {};

		return *pos;
	}
}
}
