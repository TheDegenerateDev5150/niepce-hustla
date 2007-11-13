/*
 * niepce - ui/niepcewindow.cpp
 *
 * Copyright (C) 2007 Hubert Figuiere
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

#include <iostream>

#include <glibmm/i18n.h>

#include <gtkmm/window.h>
#include <gtkmm/treestore.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/separator.h>
#include <gtkmm/treeview.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/icontheme.h>

#include "utils/debug.h"
#include "utils/moniker.h"
#include "libraryclient/libraryclient.h"
#include "framework/application.h"
#include "framework/configuration.h"

#include "eog-thumb-nav.h"
#include "eog-thumb-view.h"
#include "niepcewindow.h"
#include "librarymainviewcontroller.h"

using libraryclient::LibraryClient;
using framework::Application;
using framework::Configuration;

namespace ui {


	NiepceWindow::NiepceWindow()
		: framework::Frame()//GLADEDIR "mainwindow.glade", "mainwindow")
	{

	}

 	Gtk::Widget * 
	NiepceWindow::buildWidget()
	{
		Gtk::Window & win(gtkWindow());

		Application::Ptr pApp = Application::app();

		init_actions();
		init_ui();

		m_mainviewctrl = LibraryMainViewController::Ptr(new LibraryMainViewController());
		add(m_mainviewctrl);

		win.add(m_vbox);

		Gtk::Widget* pMenuBar = pApp->uiManager()->get_widget("/MenuBar");
		m_vbox.pack_start(*pMenuBar, Gtk::PACK_SHRINK);
		
		m_vbox.pack_start(m_hbox);


		Glib::RefPtr<Gtk::TreeStore> treestore = Gtk::TreeStore::create(m_librarycolumns);
		m_librarytree = Gtk::manage(new Gtk::TreeView());
		m_librarytree->set_model(treestore);
		Gtk::TreeModel::iterator iter = treestore->append();
		Gtk::TreeModel::Row row = *iter;
		Glib::RefPtr< Gtk::IconTheme > icon_theme(Gtk::IconTheme::get_default());
		row[m_librarycolumns.m_icon] = icon_theme->load_icon(
			Glib::ustring("folder"), 16, Gtk::ICON_LOOKUP_USE_BUILTIN);
		row[m_librarycolumns.m_label] = _("Pictures");
		row[m_librarycolumns.m_id] = 0;
		iter = treestore->append();
		row = *iter;
		row[m_librarycolumns.m_icon] = icon_theme->load_icon(
			Glib::ustring("applications-accessories"), 16, 
			Gtk::ICON_LOOKUP_USE_BUILTIN);
		row[m_librarycolumns.m_label] = _("Projects");
		row[m_librarycolumns.m_id] = 0;

		m_librarytree->set_headers_visible(true);
		m_librarytree->append_column("", m_librarycolumns.m_icon);
		m_librarytree->append_column(_("Workspace"), m_librarycolumns.m_label);

		m_hbox.set_border_width(4);
		m_hbox.pack1(*m_librarytree, Gtk::EXPAND);
	 
		
		m_hbox.pack2(*(m_mainviewctrl->widget()), Gtk::EXPAND);

		// ribbon
		GtkWidget *thv = eog_thumb_view_new();
		GtkWidget *thn = eog_thumb_nav_new(thv, EOG_THUMB_NAV_MODE_ONE_ROW, true);
		Gtk::Widget *w = Glib::wrap(thn);
		m_vbox.pack_start(*w, Gtk::PACK_SHRINK);

		// status bar
		m_vbox.pack_start(m_statusBar, Gtk::PACK_SHRINK);
		m_statusBar.push(Glib::ustring(_("Ready")));

		win.set_size_request(600, 400);
		win.show_all_children();
		on_open_library();
		return &win;
	}

	NiepceWindow::~NiepceWindow()
	{
		Application::Ptr pApp = Application::app();
		pApp->uiManager()->remove_action_group(m_refActionGroup);
	}

	void NiepceWindow::init_actions()
	{
		m_refActionGroup = Gtk::ActionGroup::create();
		
		m_refActionGroup->add(Gtk::Action::create("MenuFile", _("_File")));
		m_refActionGroup->add(Gtk::Action::create("Import", _("_Import...")),
													sigc::mem_fun(*this, 
																				&NiepceWindow::on_action_file_import));
		m_refActionGroup->add(Gtk::Action::create("Close", Gtk::Stock::CLOSE),
													sigc::mem_fun(gtkWindow(), 
																				&Gtk::Window::hide));			
		m_refActionGroup->add(Gtk::Action::create("Quit", Gtk::Stock::QUIT),
													sigc::mem_fun(Application::app().get(), 
																				&Application::quit));	
		Application::app()->uiManager()->insert_action_group(m_refActionGroup);		

		gtkWindow().add_accel_group(Application::app()->uiManager()->get_accel_group());
	}

	void NiepceWindow::on_action_file_import()
	{
		Gtk::FileChooserDialog dialog(gtkWindow(), _("Import picture folder"),
																	Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dialog.add_button(_("Import"), Gtk::RESPONSE_OK);

		int result = dialog.run();
		Glib::ustring to_import;
		switch(result)
		{
		case Gtk::RESPONSE_OK:
			to_import = dialog.get_filename();
			// pass it to the library
			// TODO
			DBG_OUT("%s", to_import.c_str());
			break;
		default:
			break;
		}
	}


	void NiepceWindow::on_action_file_quit()
	{
		Application::app()->quit();
	}


	void NiepceWindow::on_open_library()
	{
		Configuration & cfg = Application::app()->config();
		std::string libMoniker;
		libMoniker = cfg.getValue("lastOpenLibrary", "");
		if(libMoniker.empty()) {
			Gtk::FileChooserDialog dialog(gtkWindow(), _("Create library"),
																		Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
			
			dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dialog.add_button(_("Create"), Gtk::RESPONSE_OK);

			int result = dialog.run();
			Glib::ustring libraryToCreate;
			switch(result)
			{
			case Gtk::RESPONSE_OK:
				libraryToCreate = dialog.get_filename();
				// pass it to the library
				libMoniker = "local:";
				libMoniker += libraryToCreate.c_str();
				cfg.setValue("lastOpenLibrary", libMoniker);
				DBG_OUT("created library %s", libMoniker.c_str());
				break;
			default:
				break;
			}
			
		}
		else {
			DBG_OUT("last library is %s", libMoniker.c_str());
		}
		open_library(libMoniker);
	}

	void NiepceWindow::init_ui()
	{
		Application::Ptr pApp = Application::app();
		Glib::ustring ui_info =
			"<ui>"
			"  <menubar name='MenuBar'>"
			"    <menu action='MenuFile'>"
			"      <menuitem action='Import'/>"
			"      <separator/>"
			"      <menuitem action='Close'/>"
			"      <menuitem action='Quit'/>"
			"    </menu>"
//			"    <menu action='MenuEdit'>"
//			"      <menuitem action='Cut'/>"
//			"      <menuitem action='Copy'/>"
//			"      <menuitem action='Paste'/>"
//			"    </menu>"
			"  </menubar>"
			"  <toolbar  name='ToolBar'>"
			"    <toolitem action='Import'/>"
			"    <toolitem action='Quit'/>"
			"  </toolbar>"
			"</ui>";
		pApp->uiManager()->add_ui_from_string(ui_info);
	} 


	void NiepceWindow::open_library(const std::string & libMoniker)
	{
		m_libClient = LibraryClient::Ptr(new LibraryClient(utils::Moniker(libMoniker)));
		set_title(libMoniker);
	}


	void NiepceWindow::set_title(const std::string & title)
	{
		Frame::set_title(std::string(_("Niepce Digital - ")) + title);
	}


}
