/*
 * niepce - fwk/utils/gphoto.cpp
 *
 * Copyright (C) 2009 Hubert Figuiere
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


#ifndef __FWK_GPHOTO_HPP_
#define __FWK_GPHOTO_HPP_

#include <list>
#include <string>
#include <tr1/memory>

#include <boost/noncopyable.hpp>

extern "C" {
#include <gphoto2-port-info-list.h>
#include <gphoto2-abilities-list.h>
}

#include "fwk/base/singleton.hpp"

namespace fwk {

class GpDevice
  : public boost::noncopyable
{
public:
  typedef std::tr1::shared_ptr<GpDevice> Ptr;

  GpDevice(const std::string & model, const std::string & path);

  const std::string & get_model() const
    {
      return m_model;
    }
  const std::string & get_path() const
    {
      return m_path;
    }
private:
  std::string m_model;
  std::string m_path;
};



class GpDeviceList
  : public fwk::Singleton<GpDeviceList>
  , public std::list<GpDevice::Ptr>
{
public:
  ~GpDeviceList();

  void reload();
  void detect();
protected:
  friend class fwk::Singleton<GpDeviceList>;
  GpDeviceList();
private:

  void _gp_cleanup();
  ::CameraAbilitiesList *m_abilities;
  ::GPPortInfoList      *m_ports;
};



class GpCamera
{
public:
  GpCamera(const GpDevice::Ptr & device);

private:
  GpDevice::Ptr m_device;
};

}


#endif