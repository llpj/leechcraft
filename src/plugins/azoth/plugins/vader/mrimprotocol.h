/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
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

#ifndef PLUGINS_AZOTH_PLUGINS_VADER_MRIMPROTOCOL_H
#define PLUGINS_AZOTH_PLUGINS_VADER_MRIMPROTOCOL_H
#include <QObject>
#include <interfaces/iprotocol.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	class MRIMAccount;

	class MRIMProtocol : public QObject
					   , public IProtocol
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IProtocol);

		QList<MRIMAccount*> Accounts_;
	public:
		MRIMProtocol (QObject* = 0);

		QObject* GetObject ();
		ProtocolFeatures GetFeatures () const;
		QList<QObject*> GetRegisteredAccounts ();
		QObject* GetParentProtocolPlugin () const;
		QString GetProtocolName () const;
		QIcon GetProtocolIcon () const;
		QByteArray GetProtocolID () const;
		QList<QWidget*> GetAccountRegistrationWidgets (AccountAddOptions);
		void RegisterAccount (const QString&, const QList<QWidget*>&);
		QWidget* GetMUCJoinWidget ();
		void RemoveAccount (QObject*);
	private:
		void RestoreAccounts ();
	private slots:
		void saveAccounts ();
	signals:
		void accountAdded (QObject*);
		void accountRemoved (QObject*);
	};
}
}
}

#endif
