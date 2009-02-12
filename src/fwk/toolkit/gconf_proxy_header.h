/*
 * niepce - framework/gconf_proxy_header.h
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

/** @brief Wrap Gconfmm to be warning free. */

#ifndef __GCONF_PROXY_HEADER_H__
#define __GCONF_PROXY_HEADER_H__

/*
 * Insert here the work around for the warning disabling to your taste.
 */
#if __GNUC__
#pragma GCC system_header
#endif
#include <gconfmm.h>


#endif