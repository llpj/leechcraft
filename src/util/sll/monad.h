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

#include <boost/optional.hpp>
#include "oldcppkludges.h"
#include "applicative.h"

namespace LeechCraft
{
namespace Util
{
	template<typename T>
	struct InstanceMonad;

	template<template<typename...> class Monad, typename... Args, typename V>
	Monad<Args..., V> Return (const V& v)
	{
		return Pure<Monad, Args...> (v);
	}

	template<typename MV, typename F>
	auto Bind (const MV& value, const F& f)
	{
		return InstanceMonad<MV>::Bind (value, f);
	}

	// Implementations
	template<typename T>
	struct InstanceMonad<boost::optional<T>>
	{
		template<typename F>
		static ResultOf_t<F (T)> Bind (const boost::optional<T>& value, const F& f)
		{
			if (!value)
				return {};

			return f (*value);
		}
	};
}
}
