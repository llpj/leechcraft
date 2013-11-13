/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include <QDeclarativeItem>
#include <QTimer>
#include <util/utilconfig.h>

namespace LeechCraft
{
namespace Util
{
	/** @brief ToolTip for Qml objects.
	 *
	 * Using the tooltip is pretty easy.
	 * First of all register tooltip in your widget:
	 * \code{.cpp}
	 * qmlRegisterType<Util::ToolTipWidget> ("org.LC.common", 0, 1, "ToolTip");
	 * \endcode
	 *
	 * Then in yout qml import this widget:
	 * \code{.qml}
	 * import org.LC.common 0.1
	 * \endcode
	 *
	 * And now you can use tooltip:
	 * \code{.qml}
	 * Rectangle {
	 *		anchors.fill: parent
	 *
	 *		MouseArea
	 *		{
	 *			anchors.fill: subjectText
	 *			hoverEnabled: true
	 *			ToolTip
	 *			{
	 *				anchors.fill: parent
	 *				text: "tooltip text"
	 *			}
	 *		}
	 *	}
	 * \endcode
	 */
	class UTIL_API ToolTipWidget : public QDeclarativeItem
	{
		Q_OBJECT
		Q_PROPERTY (QString text READ GetText WRITE SetText NOTIFY textChanged)
		Q_PROPERTY (bool containsMouse READ IsContainsMouse NOTIFY containsMouseChanged)

		QTimer Timer_;
		QString Text_;
		bool ContainsMouse_;

	public:
		ToolTipWidget (QDeclarativeItem *parent = 0);

		void SetText (const QString& text);
		QString GetText () const;
		bool IsContainsMouse () const;

		void ShowToolTip (const QString& text) const;
	protected:
		void hoverEnterEvent (QGraphicsSceneHoverEvent *event);
		void hoverLeaveEvent (QGraphicsSceneHoverEvent *event);

	public slots:
		void timeout ();

	signals:
		void textChanged ();
		void containsMouseChanged ();
	};
}
}
