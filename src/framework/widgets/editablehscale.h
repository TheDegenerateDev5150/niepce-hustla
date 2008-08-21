/*
 * niepce - framework/widgets/editablehscale.h
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


#ifndef _FRAMEWORK_EDITABLEHSCALE_H_
#define _FRAMEWORK_EDITABLEHSCALE_H_


#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/scale.h>


namespace framework {

/** A widget similar to a Gtk::HScale with a edit box */
class EditableHScale
    : public Gtk::HBox
{
public:
    EditableHScale(double min, double max, double step);

    Gtk::Adjustment  &       get_adjustment()
        { return m_adj; }
    const Gtk::Adjustment  & get_adjustment() const
        { return m_adj; }

    sigc::signal<void,double> & signal_value_changed();
    sigc::signal<void,double> & signal_value_changing();

    bool on_button_press_event(GdkEventButton *event);

private:

    void on_adj_value_changed();

    Gtk::Adjustment  m_adj;
    Gtk::HScale      m_scale;
    Gtk::SpinButton  m_entry;
    bool             m_dirty;
    /** emitted once the value changed */
    sigc::signal<void,double> m_sig_value_changed;
    /** emitted when the value is changing (think live update) */
    sigc::signal<void,double> m_sig_value_changing;
};


}

#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0))
  indent-tabs-mode:nil
  fill-column:80
  End:
*/