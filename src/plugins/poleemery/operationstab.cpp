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

#include "operationstab.h"
#include <QStyledItemDelegate>
#include <QMessageBox>
#include "core.h"
#include "operationsmanager.h"
#include "operationpropsdialog.h"
#include "entriesmodel.h"
#include "accountsmanager.h"

namespace LeechCraft
{
namespace Poleemery
{
	namespace
	{
		class OpsDelegate : public QStyledItemDelegate
		{
		public:
			QWidget* createEditor (QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const;
			void setEditorData (QWidget*, const QModelIndex&) const;
			void setModelData (QWidget*, QAbstractItemModel*, const QModelIndex&) const;
		};

		QWidget* OpsDelegate::createEditor (QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
		{
			switch (index.column ())
			{
				case EntriesModel::Columns::Account:
					return new QComboBox (parent);
				default:
					return QStyledItemDelegate::createEditor (parent, option, index);
			}
		}

		void OpsDelegate::setEditorData (QWidget *editor, const QModelIndex& index) const
		{
			switch (index.column ())
			{
				case EntriesModel::Columns::Account:
				{
					auto box = qobject_cast<QComboBox*> (editor);
					const auto currentAccId = index.data (Qt::EditRole).toInt ();
					int toFocus = -1;
					for (const auto& acc : Core::Instance ().GetAccsManager ()->GetAccounts ())
					{
						if (acc.ID_ == currentAccId)
							toFocus = box->count ();
						box->addItem (acc.Name_, static_cast<int> (acc.ID_));
					}
					box->setCurrentIndex (toFocus);
					break;
				}
				default:
					QStyledItemDelegate::setEditorData (editor, index);
					break;
			}
		}

		void OpsDelegate::setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const
		{
			switch (index.column ())
			{
				case EntriesModel::Columns::Account:
				{
					auto box = qobject_cast<QComboBox*> (editor);
					model->setData (index, box->itemData (box->currentIndex ()));
					break;
				}
				default:
					QStyledItemDelegate::setModelData (editor, model, index);
					break;
			}
		}
	}

	OperationsTab::OperationsTab (const TabClassInfo& tc, QObject *plugin)
	: OpsManager_ (Core::Instance ().GetOpsManager ())
	, TC_ (tc)
	, ParentPlugin_ (plugin)
	{
		Ui_.setupUi (this);
		Ui_.OpsView_->setItemDelegate (new OpsDelegate);
		Ui_.OpsView_->setModel (OpsManager_->GetModel ());

		Ui_.OpsView_->resizeColumnToContents (0);
		Ui_.OpsView_->resizeColumnToContents (1);
		Ui_.OpsView_->resizeColumnToContents (2);
		Ui_.OpsView_->resizeColumnToContents (3);
	}

	TabClassInfo OperationsTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* OperationsTab::ParentMultiTabs ()
	{
		return ParentPlugin_;
	}

	void OperationsTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* OperationsTab::GetToolBar () const
	{
		return 0;
	}

	void OperationsTab::on_Add__released ()
	{
		OperationPropsDialog dia (this);
		if (dia.exec () != QDialog::Accepted)
			return;

		OpsManager_->AddEntry (dia.GetEntry ());
	}

	void OperationsTab::on_Remove__released ()
	{
		const auto& item = Ui_.OpsView_->currentIndex ();
		if (!item.isValid ())
			return;

		const auto& name = item.sibling (item.row (), EntriesModel::Columns::Name)
				.data ().toString ();
		if (QMessageBox::question (this,
					"Poleemery",
					tr ("Are you sure you want to delete entry %1?")
						.arg ("<em>" + name + "</em>"),
					QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		OpsManager_->RemoveEntry (item);
	}
}
}
