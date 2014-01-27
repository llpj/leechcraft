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

#include "backendproxy.h"
#include <QTimer>
#include <QStandardItemModel>
#include <QtDebug>
#include "linuxbackend.h"
#include "cpuloadproxyobj.h"

namespace LeechCraft
{
namespace CpuLoad
{
	namespace
	{
		class CpusModel : public QStandardItemModel
		{
		public:
			enum Role
			{
				CpuIdxRole = Qt::UserRole + 1,
				CpuLoadObj
			};

			CpusModel (QObject *parent)
			: QStandardItemModel { parent }
			{
				QHash<int, QByteArray> roleNames;
				roleNames [CpuIdxRole] = "cpuIdx";
				roleNames [CpuLoadObj] = "loadObj";
				setRoleNames (roleNames);
			}
		};
	}

	BackendProxy::BackendProxy (LinuxBackend *backend)
	: QObject { backend }
	, Backend_ { backend }
	, UpdateTimer_ { new QTimer { this } }
	, Model_ { new CpusModel { this } }
	{
		connect (UpdateTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (update ()));
		UpdateTimer_->start (1000);
	}

	QAbstractItemModel* BackendProxy::GetModel () const
	{
		return Model_;
	}

	void BackendProxy::update ()
	{
		Backend_->Update ();

		const auto rc = Model_->rowCount ();
		if (rc == Backend_->GetCpuCount ())
		{
			if (Backend_->GetCpuCount () > 0)
				for (int i = 0; i < Backend_->GetCpuCount (); ++i)
					ModelPropObjs_.at (i)->Set (Backend_->GetLoads (i));

			return;
		}

		Model_->removeRows (0, rc);
		qDeleteAll (ModelPropObjs_);
		ModelPropObjs_.clear ();

		if (Backend_->GetCpuCount () <= 0)
			return;

		QList<QStandardItem*> newItems;

		for (int i = 0; i < Backend_->GetCpuCount (); ++i)
		{
			auto obj = new CpuLoadProxyObj { Backend_->GetLoads (i) };
			ModelPropObjs_ << obj;

			auto modelItem = new QStandardItem;
			modelItem->setData (i, CpusModel::CpuIdxRole);
			modelItem->setData (QVariant::fromValue<QObject*> (obj), CpusModel::CpuLoadObj);
			newItems << modelItem;
		}

		Model_->invisibleRootItem ()->appendRows (newItems);
	}
}
}
