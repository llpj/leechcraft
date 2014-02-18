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

#include "rganalyser.h"
#include <functional>
#include <atomic>
#include <QStringList>
#include <QThread>
#include <QMetaType>
#include <QtDebug>
#include <QUrl>
#include <QWaitCondition>
#include <QMutex>
#include "gstutil.h"
#include "../gstfix.h"

namespace LeechCraft
{
namespace LMP
{
	class LightPopThread : public QThread
	{
		GstBus * const Bus_;
		QObject * const Handler_;

		std::atomic_bool ShouldStop_ { false };

		QMutex PauseMutex_;
		QWaitCondition PauseWC_;
	public:
		LightPopThread (GstBus*, QObject*);
		~LightPopThread ();

		void Stop ();
		void Resume ();
	protected:
		void run ();
	};

	LightPopThread::LightPopThread (GstBus *bus, QObject *parent)
	: QThread { parent }
	, Bus_ { bus }
	, Handler_ { parent }
	{
	}

	LightPopThread::~LightPopThread ()
	{
		gst_object_unref (Bus_);
	}

	void LightPopThread::Stop ()
	{
		ShouldStop_ = true;
	}

	void LightPopThread::Resume ()
	{
		PauseWC_.wakeOne ();
	}

	void LightPopThread::run ()
	{
		while (!ShouldStop_)
		{
			const auto msg = gst_bus_timed_pop (Bus_, GST_SECOND);
			if (!msg)
				continue;

			QMetaObject::invokeMethod (Handler_,
					"handleMessage",
					Qt::QueuedConnection,
					Q_ARG (GstMessage_ptr, std::shared_ptr<GstMessage> (msg, gst_message_unref)));

			if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
			{
				QMutexLocker locker { &PauseMutex_ };
				PauseWC_.wait (&PauseMutex_);
			}
		}
	}

	RgAnalyser::RgAnalyser (const QStringList& paths, QObject *parent)
	: QObject { parent }
	, Paths_ { paths }
#if GST_VERSION_MAJOR < 1
	, Pipeline_ (gst_element_factory_make ("playbin2", nullptr))
#else
	, Pipeline_ (gst_element_factory_make ("playbin", nullptr))
#endif
	, SinkBin_ { gst_bin_new (nullptr) }
	, AConvert_ { gst_element_factory_make ("audioconvert", nullptr) }
	, AResample_ { gst_element_factory_make ("audioresample", nullptr) }
	, RGAnalysis_ { gst_element_factory_make ("rganalysis", nullptr) }
	, Fakesink_ { gst_element_factory_make ("fakesink", nullptr) }
	, PopThread_ { new LightPopThread { gst_pipeline_get_bus (GST_PIPELINE (Pipeline_)), this } }
	{
		qRegisterMetaType<GstMessage_ptr> ("GstMessage_ptr");

		gst_bin_add_many (GST_BIN (SinkBin_),
				AConvert_, AResample_, RGAnalysis_, Fakesink_, nullptr);
		gst_element_link_many (AConvert_, AResample_, RGAnalysis_, Fakesink_, nullptr);

		auto pad = gst_element_get_static_pad (AConvert_, "sink");
		auto ghostPad = gst_ghost_pad_new ("sink", pad);
		gst_pad_set_active (ghostPad, TRUE);
		gst_element_add_pad (SinkBin_, ghostPad);
		gst_object_unref (pad);

		g_object_set (GST_OBJECT (RGAnalysis_), "num-tracks", paths.size (), nullptr);
		g_object_set (GST_OBJECT (Pipeline_), "audio-sink", SinkBin_, nullptr);

		CheckFinish ();

		PopThread_->start ();
	}

	RgAnalyser::~RgAnalyser ()
	{
		PopThread_->Stop ();
		PopThread_->wait (2000);

		gst_object_unref (Pipeline_);
	}

	const AlbumRgResult& RgAnalyser::GetResult () const
	{
		return Result_;
	}

	void RgAnalyser::CheckFinish ()
	{
		gst_element_set_state (Pipeline_, GST_STATE_NULL);

		if (Paths_.isEmpty ())
		{
			emit finished ();
			return;
		}

		CurrentPath_ = Paths_.takeFirst ();
		qDebug () << Q_FUNC_INFO << CurrentPath_;

		const auto& url = QUrl::fromLocalFile (CurrentPath_);
		g_object_set (GST_OBJECT (Pipeline_), "uri",
				url.toEncoded ().constData (), nullptr);
		gst_element_set_state (Pipeline_, GST_STATE_PLAYING);
	}

	void RgAnalyser::HandleTagMsg (GstMessage *msg)
	{
		GstUtil::TagMap_t map;
		GstUtil::ParseTagMessage (msg, map);

		auto trySet = [&map, this] (const QString& key, std::function<void (double)> setter) -> bool
		{
			const auto contains = map.contains (key);
			if (contains)
				setter (map [key].toDouble ());
			return contains;
		};
		trySet ("replaygain-album-gain", [this] (double val) { Result_.AlbumGain_ = val; });
		trySet ("replaygain-album-peak", [this] (double val) { Result_.AlbumPeak_ = val; });

		TrackRgResult track { CurrentPath_, 0, 0 };
		if (trySet ("replaygain-track-gain", [&track] (double val) { track.TrackGain_ = val; }) &&
			trySet ("replaygain-track-peak", [&track] (double val) { track.TrackPeak_ = val; }))
			Result_.Tracks_ << track;
	}

	void RgAnalyser::HandleEosMsg (GstMessage*)
	{
		CheckFinish ();
	}

	void RgAnalyser::HandleErrorMsg (GstMessage *msg)
	{
		GError *gerror = nullptr;
		gchar *debug = nullptr;
		gst_message_parse_error (msg, &gerror, &debug);

		const auto& msgStr = QString::fromUtf8 (gerror->message);
		const auto& debugStr = QString::fromUtf8 (debug);

		const auto code = gerror->code;
		const auto domain = gerror->domain;

		g_error_free (gerror);
		g_free (debug);

		qWarning () << Q_FUNC_INFO
				<< domain
				<< code
				<< msgStr
				<< debugStr;


		if (IsDraining_)
			return;

		IsDraining_ = true;
		const auto bus = gst_pipeline_get_bus (GST_PIPELINE (Pipeline_));
		while (const auto msg = gst_bus_timed_pop (bus, 0.01 * GST_SECOND))
			handleMessage (std::shared_ptr<GstMessage> (msg, gst_message_unref));
		IsDraining_ = false;

		gst_element_set_state (Pipeline_, GST_STATE_NULL);
		PopThread_->Resume ();

		const auto trackInfoPos = std::find_if (Result_.Tracks_.begin (), Result_.Tracks_.end (),
				[this] (const TrackRgResult& info) { return info.TrackPath_ == CurrentPath_; });
		if (trackInfoPos == Result_.Tracks_.end ())
			Result_.Tracks_.append ({ CurrentPath_, 0, 0 });

		CheckFinish ();
	}

	void RgAnalyser::handleMessage (GstMessage_ptr msgPtr)
	{
		const auto message = msgPtr.get ();

		switch (GST_MESSAGE_TYPE (message))
		{
		case GST_MESSAGE_TAG:
			HandleTagMsg (message);
			break;
		case GST_MESSAGE_ERROR:
			HandleErrorMsg (message);
			break;
		case GST_MESSAGE_EOS:
			HandleEosMsg (message);
			break;
		default:
			break;
		}
	}
}
}
