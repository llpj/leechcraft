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

#include <memory>
#include <QWidget>
#include <QHash>
#include <interfaces/iwkfontssettable.h>
#include "xsdconfig.h"

class QSpinBox;

namespace Ui
{
	class WkFontsWidget;
}

namespace LeechCraft
{
namespace Util
{
	class BaseSettingsManager;
	class FontChooserWidget;

	/** @brief A settings widget for configuring WebKit fonts.
	 *
	 * Provides a common widget for configuring QtWebKit fonts for
	 * standard WebKit font types.
	 *
	 * This widget works through LeechCraft's XML Settings Dialog system,
	 * storing the configuration in an BaseSettingsManager instance.
	 *
	 * This widget also supports automatically updating font settings for
	 * objects implementing the IWkFontsSettable interface if the user
	 * changes them.
	 *
	 * Typical usage includes creating an item of type
	 * <code>customwidget</code> in the XML file describing the settings
	 * and then setting an instance of the WkFontsWidget as the widget
	 * for that item. On the C++ side this looks like:
	 *  \code{.cpp}
		FontsWidget_ = new Util::WkFontsWidget { &XmlSettingsManager::Instance () };
		XmlSettingsDialog_->SetCustomWidget ("FontsSelector", FontsWidget_);
	    \endcode
	 * assuming the <code>Util::BaseSettingsManager</code> is provided by
	 * a singleton XmlSettingsManager class, and
	 * <code>XmlSettingsDialog_</code> is an instance of
	 * <code>Util::XmlSettingsDialog</code>.
	 *
	 * The code above also stores the WkFontsWidget as a class variable,
	 * which may be a good idea if one wishes to use the settings
	 * autoupdate feature. For example, assuming a class
	 * <code>ChatTab</code> properly implements the IWkFontsSettable
	 * interface:
	 * \code{.cpp}
		const auto tab = new ChatTab {};
		FontsWidget_->RegisterSettable (tab);
	   \endcode
	 *
	 * @sa IWkFontsSettable
	 */
	class UTIL_XSD_API WkFontsWidget : public QWidget
	{
		Q_OBJECT

		std::shared_ptr<Ui::WkFontsWidget> Ui_;
		BaseSettingsManager * const BSM_;

		QHash<IWkFontsSettable::FontFamily, FontChooserWidget*> Family2Chooser_;
		QHash<IWkFontsSettable::FontFamily, QByteArray> Family2Name_;
		QHash<IWkFontsSettable::FontFamily, QFont> PendingFontChanges_;

		QHash<IWkFontsSettable::FontSize, QSpinBox*> Size2Spinbox_;
		QHash<IWkFontsSettable::FontSize, QByteArray> Size2Name_;
		QHash<IWkFontsSettable::FontSize, int> PendingSizeChanges_;

		QList<IWkFontsSettable*> Settables_;

		bool IsFontZoomDirty_ = false;
	public:
		/** @brief Creates the fonts settings widget.
		 *
		 * @param[in] bsm The settings manager to use for storing
		 * settings.
		 * @param[in] parent The parent widget for this widget.
		 */
		WkFontsWidget (Util::BaseSettingsManager *bsm, QWidget *parent = nullptr);

		/** @brief Sets the tooltip for the font size multiplier control.
		 *
		 * @param[in] tooltip The tooltip string.
		 */
		void SetFontZoomTooltip (const QString& tooltip);

		/** @brief Registers an object to be automatically updated
		 * whenever font settings change.
		 *
		 * @param[in] settable An object implementing IWkFontsSettable.
		 *
		 * @sa IWkFontsSettable
		 */
		void RegisterSettable (IWkFontsSettable *settable);
	private:
		void ResetFontChoosers ();
		void ResetSizeChoosers ();
		void ResetZoom ();
	private slots:
		void on_ChangeAll__released ();
	public slots:
		void accept ();
		void reject ();
	signals:
		/** @brief Notifies the \em font for the given \em family has been
		 * changed.
		 *
		 * @param[out] family The font family for which the \em font has
		 * been changed.
		 * @param[out] font The new font for the given \em family.
		 */
		void fontChanged (IWkFontsSettable::FontFamily family, const QFont& font);

		/** @brief Notifies the \em size for the given font \em type has
		 * been changed.
		 *
		 * @param[out] type The font type for which the \em size has been
		 * changed.
		 * @param[out] size The new font size for the given font \em type.
		 */
		void sizeChanged (IWkFontsSettable::FontSize type, int size);

		/** @brief Notifies the text zoom \em factor has been changed.
		 *
		 * @param[out] factor The new font size multiplier.
		 */
		void sizeMultiplierChanged (qreal factor);
	};
}
}
