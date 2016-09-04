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

#include <QWidget>
#include <QHash>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/itexteditor.h>
#include <interfaces/iadvancedhtmleditor.h>
#include <interfaces/iwkfontssettable.h>
#include "ui_richeditorwidget.h"

class QToolBar;

namespace LeechCraft
{
namespace LHTR
{
	class RichEditorWidget : public QWidget
						   , public IEditorWidget
						   , public IAdvancedHTMLEditor
						   , public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (IEditorWidget IAdvancedHTMLEditor IWkFontsSettable)

		ICoreProxy_ptr Proxy_;
		Ui::RichEditorWidget Ui_;

		QToolBar *ViewBar_;

		QHash<QWebPage::WebAction, QAction*> WebAction2Action_;
		QHash<QString, QHash<QString, QAction*>> Cmd2Action_;

		CustomTags_t CustomTags_;

		bool HTMLDirty_;

		QAction *FindAction_;
		QAction *ReplaceAction_;

		QAction *Bold_;
		QAction *Italic_;
		QAction *Underline_;

		QAction *InsertLink_;
		QAction *InsertImage_;

		QAction *ToggleView_;
	public:
		RichEditorWidget (ICoreProxy_ptr, QWidget* = 0);

		QString GetContents (ContentType) const;
		void SetContents (QString, ContentType);
		void AppendAction (QAction*);
		void AppendSeparator ();
		void RemoveAction (QAction*);
		QAction* GetEditorAction (EditorAction);
		void SetBackgroundColor (const QColor&, ContentType);
		QWidget* GetWidget ();
		QObject* GetQObject ();

		void InsertHTML (const QString&);
		void SetCustomTags (const QList<CustomTag>&);
		QAction* AddInlineTagInserter (const QString& tagName, const QVariantMap& params);
		void ExecJS (const QString&);

		void SetFontFamily (FontFamily, const QFont&);
		void SetFontSize (FontSize, int);
		void SetFontSizeMultiplier (qreal);

		bool eventFilter (QObject*, QEvent*);
	private:
		void InternalSetBgColor (const QColor&, ContentType);

		void SetupImageMenu ();
		void SetupTableMenu ();

		void ExecCommand (const QString&, QString = QString ());
		bool QueryCommandState (const QString& cmd);

		void OpenFindReplace (bool findOnly);

		enum class ExpandMode
		{
			FullHTML,
			PartialHTML
		};
		QString ExpandCustomTags (QString, ExpandMode = ExpandMode::FullHTML) const;
		QString RevertCustomTags () const;

		void SyncHTMLToView () const;
	private slots:
		void handleBgColorSettings ();

		void handleLinkClicked (const QUrl&);
		void on_TabWidget__currentChanged (int);

		void setupJS ();

		void on_HTML__textChanged ();
		void updateActions ();

		void toggleView ();

		void handleCmd ();
		void handleInlineCmd ();
		void handleBgColor ();
		void handleFgColor ();
		void handleFont ();

		void handleInsertTable ();
		void handleInsertRow ();
		void handleInsertColumn ();
		void handleRemoveRow ();
		void handleRemoveColumn ();

		void handleInsertLink ();
		void handleInsertImage ();
		void handleCollectionImageChosen ();
		void handleInsertImageFromCollection ();

		void handleFind ();
		void handleReplace ();
	signals:
		void textChanged ();
	};
}
}
