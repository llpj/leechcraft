/**********************************************************************
 *  LeechCraft - modular cross-platform feature rich internet client.
 *  Copyright (C) 2010-2013  Oleg Linkin
 *
 *  Boost Software License - Version 1.0 - August 17th, 2003
 *
 *  Permission is hereby granted, free of charge, to any person or organization
 *  obtaining a copy of the software and accompanying documentation covered by
 *  this license (the "Software") to use, reproduce, display, distribute,
 *  execute, and transmit the Software, and to prepare derivative works of the
 *  Software, and to permit third-parties to whom the Software is furnished to
 *  do so, all subject to the following:
 *
 *  The copyright notices in the Software and this entire statement, including
 *  the above license grant, this restriction and the following disclaimer,
 *  must be included in all copies of the Software, in whole or in part, and
 *  all derivative works of the Software, unless such copies or derivative
 *  works are solely in the form of machine-executable object code generated by
 *  a source language processor.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 **********************************************************************/

#include "dummytexteditor.h"
#include <QWebFrame>

namespace LeechCraft
{
namespace Blogique
{
	DummyTextEditor::DummyTextEditor (QWidget *parent)
	: QWebView (parent)
	{
		page ()->setContentEditable (true);
		connect (page (),
				SIGNAL (contentsChanged ()),
				this,
				SIGNAL (textChanged ()));
	}

	QString DummyTextEditor::GetContents (ContentType type) const
	{
		switch (type)
		{
		case ContentType::HTML:
			return page ()->mainFrame ()->toHtml ();
		case ContentType::PlainText:
			return page ()->mainFrame ()->toPlainText ();
		}

		return QString ();
	}

	void DummyTextEditor::SetContents (QString contents, ContentType type)
	{
		switch (type)
		{
		case ContentType::HTML:
			setHtml (contents);
		case ContentType::PlainText:
			setContent (contents.toUtf8 ());
		}
	}

	void DummyTextEditor::AppendAction (QAction*)
	{
	}

	void DummyTextEditor::RemoveAction (QAction*)
	{
	}

	void DummyTextEditor::AppendSeparator ()
	{
	}

	QAction* DummyTextEditor::GetEditorAction (EditorAction)
	{
		return 0;
	}

	void DummyTextEditor::SetBackgroundColor (const QColor&, ContentType)
	{
	}

	QWidget* DummyTextEditor::GetWidget ()
	{
		return this;
	}

	QObject* DummyTextEditor::GetQObject ()
	{
		return this;
	}
}
}
