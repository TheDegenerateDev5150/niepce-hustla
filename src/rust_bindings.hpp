/*
 * niepce - rust_bindings.hpp
 *
 * Copyright (C) 2017-2024 Hubert Figuière
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

#pragma once

#define _IN_RUST_BINDINGS_

#include <memory>
#include <vector>

#include <gtk/gtk.h>

#include "fwk/cxx_fwk_bindings.hpp"
#include "fwk/cxx_eng_bindings.hpp"
#include "fwk/cxx_ncr_bindings.hpp"
#include "fwk/cxx_npc_bindings.hpp"

namespace ffi {
struct Utc;
template <class T>
struct DateTime;
typedef fwk::Date Date;
typedef fwk::RgbColour RgbColour;
typedef eng::Label Label;
typedef eng::LibraryClientWrapper LibraryClientWrapper;
}

namespace fwk {
typedef rust::Box<RgbColour> RgbColourPtr;

typedef rust::Box<PropertyBag> PropertyBagPtr;
}

namespace eng {
typedef rust::Box<Label> LabelPtr;
typedef std::vector<LabelPtr> LabelList;
typedef int64_t library_id_t;
typedef rust::Box<LibraryClientHost> LibraryClientPtr;
}

#undef _IN_RUST_BINDINGS_
