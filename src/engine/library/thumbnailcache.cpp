/*
 * niepce - library/thumbnailcache.cpp
 *
 * Copyright (C) 2007-2008 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <functional>
#include <boost/bind.hpp>
#include <boost/any.hpp>

#include <gdkmm/pixbuf.h>
#include <libopenraw-gnome/gdkpixbuf.h>

#include "niepce/notifications.hpp"
#include "fwk/base/debug.hpp"
#include "fwk/toolkit/mimetype.hpp"
#include "fwk/toolkit/gdkutils.hpp"
#include "thumbnailcache.hpp"
#include "thumbnailnotification.hpp"

namespace eng {

	ThumbnailCache::ThumbnailCache(const std::string & dir,
								   const fwk::NotificationCenter::Ptr & nc)
		: m_cacheDir(dir),
		  m_notif_center(nc)
	{
	}

	ThumbnailCache::~ThumbnailCache()
	{
	}

	void ThumbnailCache::request(const LibFile::ListPtr & fl)
	{
		clear();
		std::for_each(fl->begin(), fl->end(),
					 boost::bind(&ThumbnailCache::requestForFile, this, 
								 _1));
	}

	void ThumbnailCache::requestForFile(const LibFile::Ptr & f)
	{
		ThumbnailTask::Ptr task(new ThumbnailTask(f, 160, 160));
		schedule( task );
	}


	void ThumbnailCache::execute(const  ThumbnailTask::Ptr & task)
	{
    const std::string & filename = task->file()->path();
		DBG_OUT("creating thumbnail for %s",filename.c_str());
    int w, h;
    w = task->width();
    h = task->height();

		fwk::MimeType mime_type(filename);


		DBG_OUT("MIME type %s", mime_type.string().c_str());

		if(mime_type.isUnknown()) {
			DBG_OUT("unknown file type", filename.c_str());
			return;
		}
		if(!mime_type.isImage()) {
			DBG_OUT("not an image type");
			return;
		}
		
		Glib::RefPtr<Gdk::Pixbuf> pix;
		if(!mime_type.isDigicamRaw()) {
			DBG_OUT("not a raw type, trying GdkPixbuf loaders");
            try {
                pix = Gdk::Pixbuf::create_from_file(filename, w, h, true);
                if(pix) {
                    pix = fwk::gdkpixbuf_exif_rotate(pix, task->file()->orientation());
                }
            }
            catch(const Glib::Error & e) 
            {
                ERR_OUT("exception %s", e.what().c_str());
            }
		}	
		else {	
			GdkPixbuf *pixbuf = or_gdkpixbuf_extract_rotated_thumbnail(filename.c_str(), 
																	   std::min(w, h));
			if(pixbuf) {
				 pix = Glib::wrap(pixbuf, true); // take ownership
			}
		}
		if(pix)
		{
            if((w < pix->get_width()) || (h < pix->get_height())) {
                pix = fwk::gdkpixbuf_scale_to_fit(pix, std::min(w,h));
            }
			fwk::NotificationCenter::Ptr nc(m_notif_center);
			if(nc) {
				// pass the notification
				fwk::Notification::Ptr n(new fwk::Notification(niepce::NOTIFICATION_THUMBNAIL));
				ThumbnailNotification tn;
				tn.id = task->file()->id();
				tn.width = pix->get_width();
				tn.height = pix->get_height();
				tn.pixmap = pix;
				n->setData(boost::any(tn));
				DBG_OUT("notify thumbnail for id=%d", tn.id);
				nc->post(n);
			}
		}
		else 
		{
			DBG_OUT("couldn't get the thumbnail for %s", filename.c_str());
		}
	}

}
