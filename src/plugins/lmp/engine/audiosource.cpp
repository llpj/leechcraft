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

#include "audiosource.h"
#include <QUrl>

namespace LeechCraft
{
namespace LMP
{
	AudioSource::AudioSource ()
	{
	}

	AudioSource::AudioSource (const QString& filename)
	: Url_ (QUrl::fromLocalFile (filename))
	{
	}

	AudioSource::AudioSource (const QUrl& url)
	: Url_ (url)
	{
	}

	AudioSource::AudioSource (const AudioSource& source)
	: Url_ (source.Url_)
	{
	}

	AudioSource& AudioSource::operator= (const AudioSource& source)
	{
		Url_ = source.Url_;
		return *this;
	}

	bool AudioSource::operator== (const AudioSource& source) const
	{
		return Url_ == source.Url_;
	}

	bool AudioSource::operator!= (const AudioSource& source) const
	{
		return !(*this == source);
	}

	QUrl AudioSource::ToUrl () const
	{
		return Url_;
	}

	bool AudioSource::IsLocalFile () const
	{
		return Url_.scheme () == "file";
	}

	QString AudioSource::GetLocalPath () const
	{
		return Url_.toLocalFile ();
	}

	bool AudioSource::IsRemote () const
	{
		if (IsEmpty ())
			return false;

		return !IsLocalFile ();
	}

	bool AudioSource::IsEmpty () const
	{
		return !Url_.isValid ();
	}

	void AudioSource::Clear ()
	{
		Url_.clear ();
	}

	AudioSource::Type AudioSource::GetType () const
	{
		return IsLocalFile () ? Type::File : Type::Url;
	}

	uint qHash (const AudioSource& source)
	{
		return qHash (source.ToUrl ());
	}
}
}
