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

#include "core.h"
#include <algorithm>
#include <QFile>
#include <interfaces/iplugin2.h>
#include "pixmapcachemanager.h"
#include "recentlyopenedmanager.h"
#include "defaultbackendmanager.h"
#include "docstatemanager.h"
#include "bookmarksmanager.h"
#include "interfaces/monocle/iredirectproxy.h"

namespace LeechCraft
{
namespace Monocle
{
	Core::Core ()
	: CacheManager_ (new PixmapCacheManager (this))
	, ROManager_ (new RecentlyOpenedManager (this))
	, DefaultBackendManager_ (new DefaultBackendManager (this))
	, DocStateManager_ (new DocStateManager (this))
	, BookmarksManager_ (new BookmarksManager (this))
	{
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		DefaultBackendManager_->LoadSettings ();
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	void Core::AddPlugin (QObject *pluginObj)
	{
		auto plugin2 = qobject_cast<IPlugin2*> (pluginObj);
		const auto& classes = plugin2->GetPluginClasses ();
		if (classes.contains ("org.LeechCraft.Monocle.IBackendPlugin"))
			Backends_ << pluginObj;
	}

	bool Core::CanHandleMime (const QString& mime)
	{
		return std::any_of (Backends_.begin (), Backends_.end (),
				[&mime] (QObject *plugin)
				{
					return qobject_cast<IBackendPlugin*> (plugin)->
							GetSupportedMimes ().contains (mime);
				});
	}

	bool Core::CanLoadDocument (const QString& path)
	{
		decltype (Backends_) redirectors;
		for (auto backend : Backends_)
		{
			const auto ibp = qobject_cast<IBackendPlugin*> (backend);
			switch (ibp->CanLoadDocument (path))
			{
			case IBackendPlugin::LoadCheckResult::Can:
				return true;
			case IBackendPlugin::LoadCheckResult::Redirect:
				redirectors << backend;
				break;
			case IBackendPlugin::LoadCheckResult::Cannot:
				break;
			}
		}

		return std::any_of (redirectors.begin (), redirectors.end (),
				[&path, this] (QObject *redirectorObj) -> bool
				{
					const auto redirector = qobject_cast<IBackendPlugin*> (redirectorObj);
					const auto redirect = redirector->GetRedirection (path);
					return CanHandleMime (redirect->GetRedirectedMime ());
				});
	}

	IDocument_ptr Core::LoadDocument (const QString& path)
	{
		if (!QFile::exists (path))
			return IDocument_ptr ();

		decltype (Backends_) loaders;
		decltype (Backends_) redirectors;
		for (auto backend : Backends_)
		{
			switch (qobject_cast<IBackendPlugin*> (backend)->CanLoadDocument (path))
			{
			case IBackendPlugin::LoadCheckResult::Can:
				loaders << backend;
				break;
			case IBackendPlugin::LoadCheckResult::Redirect:
				redirectors << backend;
				break;
			case IBackendPlugin::LoadCheckResult::Cannot:
				break;
			}
		}

		if (loaders.isEmpty ())
			return IDocument_ptr ();
		else if (loaders.size () == 1)
			return qobject_cast<IBackendPlugin*> (loaders.at (0))->LoadDocument (path);

		auto backend = DefaultBackendManager_->GetBackend (loaders);
		return backend ?
				qobject_cast<IBackendPlugin*> (backend)->LoadDocument (path) :
				IDocument_ptr ();
	}

	PixmapCacheManager* Core::GetPixmapCacheManager () const
	{
		return CacheManager_;
	}

	RecentlyOpenedManager* Core::GetROManager () const
	{
		return ROManager_;
	}

	DefaultBackendManager* Core::GetDefaultBackendManager () const
	{
		return DefaultBackendManager_;
	}

	DocStateManager* Core::GetDocStateManager () const
	{
		return DocStateManager_;
	}

	BookmarksManager* Core::GetBookmarksManager () const
	{
		return BookmarksManager_;
	}
}
}
