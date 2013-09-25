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
#include <QPointer>
#include <QHash>
#include <QUrl>
#include <interfaces/core/icoreproxy.h>

class QUrl;
class QPoint;

namespace LeechCraft
{
namespace SB2
{
	class ViewManager;
	class DeclarativeWindow;
	class QuarkOrderView;

	class QuarkProxy : public QObject
	{
		Q_OBJECT
		Q_PROPERTY (QString extHoveredQuarkClass READ GetExtHoveredQuarkClass NOTIFY extHoveredQuarkClassChanged)

		ViewManager *Manager_;
		ICoreProxy_ptr Proxy_;

		QString ExtHoveredQuarkClass_;

		QHash<QUrl, QPointer<DeclarativeWindow>> URL2LastOpened_;
		QPointer<QuarkOrderView> QuarkOrderView_;
	public:
		QuarkProxy (ViewManager*, ICoreProxy_ptr, QObject* = 0);

		const QString& GetExtHoveredQuarkClass () const;
		QRect GetFreeCoords () const;
	public slots:
		QPoint mapToGlobal (double, double);
		void showTextTooltip (int, int, const QString&);
		void showSettings (const QUrl&);
		void removeQuark (const QUrl&);
		QVariant openWindow (const QUrl&, const QString&, const QVariant&);
		QRect getWinRect ();

		void panelMoveRequested (const QString&);

		void quarkAddRequested (int, int);
		void quarkOrderRequested (int, int);

		void panelSettingsRequested ();
	private slots:
		void handleExtHoveredQuarkClass (const QString&);
	signals:
		void extHoveredQuarkClassChanged ();
	};
}
}
