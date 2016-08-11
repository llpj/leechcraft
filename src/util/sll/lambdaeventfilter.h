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

#include <QObject>
#include "oldcppkludges.h"

namespace LeechCraft
{
namespace Util
{
	namespace detail
	{
		template<typename F, typename R>
		F TypeGetter (R (*) (F));

		template<typename F>
		auto TypeGetter (F&& f) -> decltype (TypeGetter (+f));

		template<typename C, typename R, typename F>
		F TypeGetter (R (C::*) (F) const);

		template<typename C, typename R, typename F>
		F TypeGetter (R (C::*) (F));

		template<typename C>
		decltype (TypeGetter (&C::operator ())) TypeGetter (const C& c);

		template<typename F>
		using ArgType_t = decltype (TypeGetter (*static_cast<F*> (nullptr)));

		template<typename F>
		class LambdaEventFilter : public QObject
		{
			const F F_;

			using EventType_t = typename std::remove_pointer<ArgType_t<F>>::type;
		public:
			LambdaEventFilter (F&& f, QObject *parent = nullptr)
					: QObject { parent }
					, F_ { std::move (f) }
			{
			}

			bool eventFilter (QObject*, QEvent *srcEv) override
			{
				const auto ev = dynamic_cast<EventType_t*> (srcEv);
				if (!ev)
					return false;

				return F_ (ev);
			}
		};
	}

	template<typename F>
	auto MakeLambdaEventFilter (F&& f, QObject *parent = nullptr)
	{
		return new detail::LambdaEventFilter<Util::Decay_t<F>> { std::forward<F> (f), parent };
	}
}
}