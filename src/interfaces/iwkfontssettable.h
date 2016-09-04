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

#include <QtPlugin>

class QFont;

/** @brief Interface to aid WebKit-like-view-containing tabs to expose the view
 * fonts configuration to the user.
 *
 * The tabs implementing this interface should just be registered with an
 * instance of LeechCraft::Util::WkFontsWidget, which will take care of
 * the rest.
 *
 * @sa ITabWidget
 */
class Q_DECL_EXPORT IWkFontsSettable
{
protected:
	virtual ~IWkFontsSettable () = default;
public:
	enum class FontFamily
	{
		StandardFont,
		FixedFont,
		SerifFont,
		SansSerifFont,
		CursiveFont,
		FantasyFont
	};

	enum class FontSize
	{
		MinimumFontSize,
		MinimumLogicalFontSize,
		DefaultFontSize,
		DefaultFixedFontSize
	};

	/** @brief Returns this tab as a QObject.
	 *
	 * @return This tab as a QObject.
	 */
	virtual QObject* GetQObject () = 0;

	/** @brief Sets the \em font for the given font \em family.
	 *
	 * See also <code>QWebSettings::setFontFamily()</code>.
	 *
	 * @param[in] family The font family to change.
	 * @param[in] font The font to set for the font family.
	 */
	virtual void SetFontFamily (FontFamily family, const QFont& font) = 0;

	/** @brief Sets the \em size for the given font size \em type.
	 *
	 * See also <code>QWebSettings::setFontSize()</code>.
	 *
	 * @param[in] type The font type to change.
	 * @param[in] size The font size to set.
	 */
	virtual void SetFontSize (FontSize type, int size) = 0;

	/** @brief Sets the font size multiplier to the given \em factor.
	 *
	 * See also <code>QWebView::setTextSizeMultiplier()</code>.
	 *
	 * @param[in] factor The font size multiplier.
	 */
	virtual void SetFontSizeMultiplier (qreal factor) = 0;
};

inline uint qHash (IWkFontsSettable::FontFamily f)
{
	return static_cast<uint> (f);
}

inline uint qHash (IWkFontsSettable::FontSize f)
{
	return static_cast<uint> (f);
}

Q_DECLARE_INTERFACE (IWkFontsSettable, "org.LeechCraft.IWkFontsSettable/1.0")
