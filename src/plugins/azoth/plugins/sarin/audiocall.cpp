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

#include "audiocall.h"
#include <QFuture>
#include <QFutureWatcher>
#include <util/sll/functional.h>
#include <util/sll/slotclosure.h>
#include <util/threads/futures.h>
#include "threadexceptions.h"
#include "audiocalldevice.h"
#include "toxcontact.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	AudioCall::AudioCall (const ToxContact *contact, CallManager *callMgr, Direction dir)
	: SourceId_ { contact->GetEntryID () }
	, SourcePubkey_ { contact->GetPubKey () }
	, Dir_ { dir }
	, CallMgr_ { callMgr }
	{
		connect (CallMgr_,
				SIGNAL (callStateChanged (int32_t, uint32_t)),
				this,
				SLOT (handleCallStateChanged (int32_t, uint32_t)));
	}

	void AudioCall::SetCallIdx (const boost::optional<qint32>& idx)
	{
		if (!idx)
		{
			qWarning () << Q_FUNC_INFO
					<< "no call idx";
			emit stateChanged (SFinished);
			return;
		}

		CallIdx_ = *idx;

		Device_ = std::make_shared<AudioCallDevice> (CallIdx_, CallMgr_);
		Device_->open (QIODevice::ReadWrite);

		if (Dir_ == DOut)
			InitiateCall ();
	}

	IMediaCall::Direction AudioCall::GetDirection () const
	{
		return Dir_;
	}

	QString AudioCall::GetSourceID () const
	{
		return SourceId_;
	}

	void AudioCall::Accept ()
	{
		if (Dir_ == DOut)
			return;

		try
		{
			CallMgr_->AcceptCall (CallIdx_).result ();
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();

			emit stateChanged (IMediaCall::SFinished);
		}
	}

	void AudioCall::Hangup ()
	{
	}

	QIODevice* AudioCall::GetAudioDevice ()
	{
		return Device_.get ();
	}

	QAudioFormat AudioCall::GetAudioFormat ()
	{
		return ReadFmt_;
	}

	QIODevice* AudioCall::GetVideoDevice ()
	{
		return nullptr;
	}

	void AudioCall::InitiateCall ()
	{
		Util::Sequence (this, CallMgr_->InitiateCall (SourcePubkey_.toUtf8 ())) >>
				[this] (const CallManager::InitiateResult& result)
				{
					if (result)
					{
						qWarning () << Q_FUNC_INFO
								<< "error initiating the call";
						emit stateChanged (SFinished);
						return;
					}

					emit stateChanged (SConnecting);
				};
	}

	void AudioCall::handleCallStateChanged (int32_t callIdx, uint32_t state)
	{
		if (callIdx != CallIdx_)
			return;

		qDebug () << Q_FUNC_INFO;

		/*
		ReadFmt_.setChannelCount ();
		ReadFmt_.setSampleRate ();
		*/
		ReadFmt_.setSampleSize (16);
		ReadFmt_.setByteOrder (QSysInfo::ByteOrder == QSysInfo::BigEndian ?
				QAudioFormat::BigEndian :
				QAudioFormat::LittleEndian);
		ReadFmt_.setCodec ("audio/pcm");
		ReadFmt_.setSampleType (QAudioFormat::SignedInt);

		emit stateChanged (SActive);
		emit audioModeChanged (QIODevice::ReadWrite);
	}
}
}
}
