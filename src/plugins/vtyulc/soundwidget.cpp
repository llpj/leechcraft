/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include <vlc/vlc.h>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include "soundwidget.h"

namespace LeechCraft
{
namespace vlc
{
	SoundWidget::SoundWidget (QWidget *parent, std::shared_ptr<libvlc_media_player_t> mp)
	: QWidget (parent)
	, Mp_ (mp)
	{		
		setVolume (100);
		connect (this,
				SIGNAL (volumeChanged (int)),
				this,
				SLOT (update ()));
	}
	
	void SoundWidget::decreaseVolume ()
	{
		setVolume (libvlc_audio_get_volume (Mp_.get ()) - 10);
	}
	
	void SoundWidget::increaseVolume ()
	{
		setVolume (libvlc_audio_get_volume (Mp_.get ()) + 10);
	}
	
	void SoundWidget::setVolume (int volume)
	{
		volume = std::min (std::max (volume, 0), 200);
		volume -= volume % 10;
		libvlc_audio_set_volume (Mp_.get (), volume);
		emit volumeChanged (libvlc_audio_get_volume (Mp_.get ()));
	}
	
	void SoundWidget::mousePressEvent (QMouseEvent *event) 
	{
		setVolume (event->x () * 200 / width ());
	}
	
	void SoundWidget::paintEvent (QPaintEvent *event)
	{
		QPainter p (this);
		QPen goodPen = p.pen ();
		
		const int h = height () - 1;
		const int w = width ();
		
		int currentVolume = libvlc_audio_get_volume (Mp_.get ());
		if (currentVolume == -1)
			currentVolume = 100;
		
		for (int i = 1; i <= currentVolume; i++) 
		{
			if (i <= 100)
				p.setPen (QColor (20, 150 + i, 20));
			else
				p.setPen (QColor (255, 255 - (i - 100) * 2.5, 10));
			
			p.drawLine (i / 2, h - h * i / 200 - 1, i / 2, h - 1);
		}
		
		p.setPen (goodPen);
		
		p.drawLine (0, h - 1, w - 1, h - 1);
		p.drawLine (w - 1, 0, w - 1, h - 1);
		p.drawLine (0, h - 1, w - 1, 0);
		
		QFont painterFont = font ();
		painterFont.setPointSize (8);
		p.setFont (painterFont);
		p.drawText (3, 13, QString::number (currentVolume) + "%");
			
		p.end ();
		event->accept ();
	}
	
	void SoundWidget::wheelEvent (QWheelEvent *event)
	{
		if (event->delta () > 0)
			increaseVolume ();
		else
			decreaseVolume ();
	}

}
}
