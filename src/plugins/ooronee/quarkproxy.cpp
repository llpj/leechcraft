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

#include "quarkproxy.h"
#include <QInputDialog>
#include <util/util.h>
#include <util/xpc/stddatafiltermenucreator.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/idatafilter.h>
#include <interfaces/ientityhandler.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Ooronee
{
	QuarkProxy::QuarkProxy (ICoreProxy_ptr proxy)
	: Proxy_ { proxy }
	{
		XmlSettingsManager::Instance ().RegisterObject ("HoverTimeout",
				this, "hoverTimeoutChanged");
	}

	int QuarkProxy::GetHoverTimeout () const
	{
		return XmlSettingsManager::Instance ().property ("HoverTimeout").toInt ();
	}

	void QuarkProxy::Handle (const QVariant& data, const QByteArray& typeId, bool menuSelect)
	{
		if (menuSelect)
		{
			HandleVariantsMenu (data, typeId);
			return;
		}

		const auto getId = [&typeId, &menuSelect] (const QByteArray& prefix) -> QByteArray
		{
			return XmlSettingsManager::Instance ()
				.Property (prefix + typeId, {}).toByteArray ();
		};
		const auto& prevPluginId = getId ("PrevHandler");
		const auto& prevVariantId = getId ("PrevVariant");

		auto entity = Util::MakeEntity (data,
				{},
				TaskParameter::NoParameters,
				"x-leechcraft/data-filter-request");

		QList<VarInfo> varInfos;

		const auto& objs = Proxy_->GetEntityManager ()->GetPossibleHandlers (entity);
		for (const auto& obj : objs)
		{
			const auto& pluginId = qobject_cast<IInfo*> (obj)->GetUniqueID ();
			const auto idf = qobject_cast<IDataFilter*> (obj);

			for (const auto& var : idf->GetFilterVariants ())
			{
				if (pluginId == prevPluginId &&
						var.ID_ == prevVariantId)
				{
					entity.Additional_ ["DataFilter"] = var.ID_;
					qobject_cast<IEntityHandler*> (obj)->Handle (entity);

					return;
				}

				varInfos.append ({
						idf->GetFilterVerb () + ": " + var.Name_,
						obj,
						var.ID_
					});
			}
		}

		QStringList strings;
		for (const auto& info : varInfos)
			strings << info.HumanReadable_;

		HandleVariantsDialog (entity, strings, varInfos, typeId);
	}

	void QuarkProxy::HandleVariantsMenu (const QVariant& data, const QByteArray& typeId)
	{
		QMenu menu;
		Util::StdDataFilterMenuCreator creator { data, Proxy_->GetEntityManager (), &menu };
		menu.exec (QCursor::pos ());

		SaveUsed (creator.GetChosenPlugin (), creator.GetChosenVariant (), typeId);
	}

	void QuarkProxy::HandleVariantsDialog (Entity entity,
			const QStringList& strings, const QList<VarInfo>& varInfos, const QByteArray& typeId)
	{
		bool ok = false;
		const auto& selected = QInputDialog::getItem (nullptr,
				tr ("Handle text"),
				tr ("Select the data filter to handle the dropped text:"),
				strings,
				0,
				false,
				&ok);
		if (!ok)
			return;

		const auto idx = strings.indexOf (selected);
		if (idx < 0)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot find"
					<< idx;
			return;
		}

		const auto& varInfo = varInfos.at (idx);

		entity.Additional_ ["DataFilter"] = varInfo.Variant_;
		qobject_cast<IEntityHandler*> (varInfo.Obj_)->Handle (entity);

		SaveUsed (qobject_cast<IInfo*> (varInfo.Obj_)->GetUniqueID (), varInfo.Variant_, typeId);
	}

	void QuarkProxy::SaveUsed (const QByteArray& plugin,
			const QByteArray& variant, const QByteArray& typeId)
	{
		XmlSettingsManager::Instance ().setProperty ("PrevHandler" + typeId, plugin);
		XmlSettingsManager::Instance ().setProperty ("PrevVariant" + typeId, variant);
	}

	void QuarkProxy::handle (const QVariant& data, bool menuSelect)
	{
		Handle (data, data.canConvert<QImage> () ? "Image" : "Text", menuSelect);
	}
}
}
