/*
 * niepce - framework/application.h
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


#ifndef _FRAMEWORK_APPLICATION_H_
#define _FRAMEWORK_APPLICATION_H_

#include <boost/function.hpp>

#include <glibmm/refptr.h>
#include <gtkmm/uimanager.h>

#include "framework/configuration.h"
#include "framework/frame.h"

namespace framework {

	class Application 
		: public Controller
	{
	public:
		typedef boost::shared_ptr<Application> Ptr;

		virtual ~Application();

		virtual Frame::Ptr makeMainFrame() = 0;

		Configuration & config()
			{ return m_config; }
		Glib::RefPtr<Gtk::UIManager> uiManager()
			{ 
				if(!m_refUIManager) {
					m_refUIManager = Gtk::UIManager::create();
				}
				return m_refUIManager; 
			}

		virtual Gtk::Widget * buildWidget();
		virtual void quit();
		virtual void add(const Controller::Ptr & sub);

		static Application::Ptr app();
		static int main(boost::function<Application::Ptr (void)> constructor, 
										int argc, char **argv);

		
	protected:
		Application();
		static Application::Ptr m_application; 

	private:
		Configuration                m_config;
		Glib::RefPtr<Gtk::UIManager> m_refUIManager;
	};

}

#endif