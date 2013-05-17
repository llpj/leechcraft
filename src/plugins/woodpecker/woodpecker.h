/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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
#include <interfaces/iinfo.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/structures.h>

class QTranslator;

namespace LeechCraft
{
namespace Woodpecker
{
	class Plugin 
		: public QObject
		  , public IInfo
		  , public IHaveTabs
		  , public IHaveSettings
	{
			Q_OBJECT
			Q_INTERFACES (IInfo IHaveTabs IHaveSettings)

			QList<QPair<TabClassInfo, std::function<void (TabClassInfo)>>> TabClasses_;
			std::shared_ptr<QTranslator> Translator_;
			std::shared_ptr<Util::XmlSettingsDialog> XmlSettingsDialog_;
		public:
			void Init (ICoreProxy_ptr);
			void SecondInit ();
			void Release ();
			QByteArray GetUniqueID () const;
			QString GetName () const;
			QString GetInfo () const;
			QIcon GetIcon () const;

			TabClasses_t GetTabClasses () const;
			void TabOpenRequested (const QByteArray&);

			std::shared_ptr<Util::XmlSettingsDialog> GetSettingsDialog () const;
			
		private:
			void MakeTab (QWidget*, const TabClassInfo&);

		signals:
			void addNewTab (const QString&, QWidget*);
			void removeTab (QWidget*);
			void changeTabName (QWidget*, const QString&);
			void changeTabIcon (QWidget*, const QIcon&);
			void changeTooltip (QWidget*, QWidget*);
			void statusBarChanged (QWidget*, const QString&);
			void raiseTab (QWidget*);
			void delegateEntity (const LeechCraft::Entity&,
					int*, QObject**);
			void gotEntity (const LeechCraft::Entity&);

			void couldHandle (const LeechCraft::Entity&, bool*);
	};
};
};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
