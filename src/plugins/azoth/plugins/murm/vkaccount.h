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

#pragma once

#include <QObject>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/isupporttune.h>
#include <interfaces/azoth/iextselfinfoaccount.h>
#include <interfaces/core/icoreproxy.h>
#include "structures.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	class VkEntry;
	class VkChatEntry;
	class VkMessage;
	class VkProtocol;
	class VkConnection;
	class PhotoStorage;
	class GeoResolver;
	class GroupsManager;
	class Logger;

	class VkAccount : public QObject
					, public IAccount
					, public ISupportTune
					, public IExtSelfInfoAccount
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IAccount
				LeechCraft::Azoth::ISupportTune
				LeechCraft::Azoth::IExtSelfInfoAccount)

		const ICoreProxy_ptr CoreProxy_;

		VkProtocol * const Proto_;
		const QByteArray ID_;

		PhotoStorage * const PhotoStorage_;

		QString Name_;

		Logger * const Logger_;

		VkConnection * const Conn_;
		GroupsManager * const GroupsMgr_;
		GeoResolver * const GeoResolver_;

		VkEntry *SelfEntry_ = nullptr;
		QHash<qulonglong, VkEntry*> Entries_;
		QHash<qulonglong, VkChatEntry*> ChatEntries_;

		QList<MessageInfo> PendingMessages_;
	public:
		VkAccount (const QString& name, VkProtocol *proto, ICoreProxy_ptr proxy,
				const QByteArray& id, const QByteArray& cookies);

		QByteArray Serialize () const;
		static VkAccount* Deserialize (const QByteArray&, VkProtocol*, ICoreProxy_ptr);

		void Send (VkEntry*, VkMessage*);
		void Send (VkChatEntry*, VkMessage*);
		void CreateChat (const QString&, const QList<VkEntry*>&);
		VkEntry* GetEntry (qulonglong) const;
		VkEntry* GetSelf () const;

		ICoreProxy_ptr GetCoreProxy () const;
		VkConnection* GetConnection () const;
		PhotoStorage* GetPhotoStorage () const;
		GeoResolver* GetGeoResolver () const;
		GroupsManager* GetGroupsManager () const;

		QObject* GetQObject ();
		QObject* GetParentProtocol () const;
		AccountFeatures GetAccountFeatures () const;
		QList<QObject*> GetCLEntries ();

		QString GetAccountName () const;
		QString GetOurNick () const;
		void RenameAccount (const QString& name);
		QByteArray GetAccountID () const;
		QList<QAction*> GetActions () const;

		void QueryInfo (const QString& address);
		void OpenConfigurationDialog ();

		EntryStatus GetState () const;

		void ChangeState (const EntryStatus&);
		void Authorize (QObject*);
		void DenyAuth (QObject*);
		void RequestAuth (const QString&, const QString&, const QString&, const QStringList&);
		void RemoveEntry (QObject*);
		QObject* GetTransferManager () const;

		void PublishTune (const QMap<QString, QVariant>& tuneData);

		QObject* GetSelfContact () const;
		QImage GetSelfAvatar () const;
		QIcon GetAccountIcon () const;
	private slots:
		void handleSelfInfo (const UserInfo&);
		void handleUsers (const QList<UserInfo>&);
		void handleUserState (qulonglong, bool);
		void handleMessage (const MessageInfo&);
		void handleTypingNotification (qulonglong);

		void handleGotChatInfo (const ChatInfo&);
		void handleChatUserRemoved (qulonglong, qulonglong);

		void handleRemoveEntry (VkChatEntry*);

		void handleMarkOnline ();

		void finishOffline ();

		void emitUpdateAcc ();
	signals:
		void accountRenamed (const QString&);
		void gotCLItems (const QList<QObject*>&);
		void removedCLItems (const QList<QObject*>&);
		void authorizationRequested (QObject*, const QString&);
		void itemSubscribed (QObject*, const QString&);
		void itemUnsubscribed (QObject*, const QString&);
		void itemUnsubscribed (const QString&, const QString&);
		void itemCancelledSubscription (QObject*, const QString&);
		void itemGrantedSubscription (QObject*, const QString&);
		void statusChanged (const EntryStatus&);
		void mucInvitationReceived (const QVariantMap&, const QString&, const QString&);

		void accountChanged (VkAccount*);
	};
}
}
}
