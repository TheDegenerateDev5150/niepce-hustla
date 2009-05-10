/*
 * niepce - ui/librarycellrenderer.h
 *
 * Copyright (C) 2008 Hubert Figuiere
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



#ifndef __UI_LIBRARY_CELL_RENDERER_H__
#define __UI_LIBRARY_CELL_RENDERER_H__

#include <gtkmm/cellrendererpixbuf.h>
#include <cairomm/surface.h>

#include "engine/db/libfile.hpp"

namespace ui {

class LibraryCellRenderer
	: public Gtk::CellRendererPixbuf
{
public:
	LibraryCellRenderer();

	virtual void 	get_size_vfunc (Gtk::Widget& widget, 
									const Gdk::Rectangle* cell_area, 
									int* x_offset, int* y_offset, 
									int* width, int* height) const;
	virtual void 	render_vfunc (const Glib::RefPtr<Gdk::Drawable>& window, 
								  Gtk::Widget& widget, 
								  const Gdk::Rectangle& background_area, 
								  const Gdk::Rectangle& cell_area, 
								  const Gdk::Rectangle& expose_area, 
								  Gtk::CellRendererState flags);

    void set_size(int _size)
        { m_size = _size; }
    int size() const
        { return m_size; }

	Glib::PropertyProxy_ReadOnly<eng::LibFile::Ptr> 	property_libfile() const;
	Glib::PropertyProxy<eng::LibFile::Ptr> 	property_libfile();

private:
    int                                 m_size;
	Glib::Property<eng::LibFile::Ptr>    m_libfileproperty;

	Cairo::RefPtr<Cairo::ImageSurface>  m_raw_format_emblem;
	Cairo::RefPtr<Cairo::ImageSurface>  m_rawjpeg_format_emblem;
	Cairo::RefPtr<Cairo::ImageSurface>  m_img_format_emblem;
	Cairo::RefPtr<Cairo::ImageSurface>  m_video_format_emblem;
	Cairo::RefPtr<Cairo::ImageSurface>  m_unknown_format_emblem;

	Cairo::RefPtr<Cairo::ImageSurface>  m_star;
	Cairo::RefPtr<Cairo::ImageSurface>  m_unstar;
};


}

#endif
