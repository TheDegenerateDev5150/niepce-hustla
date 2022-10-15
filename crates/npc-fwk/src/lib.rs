/*
 * niepce - fwk/lib.rs
 *
 * Copyright (C) 2017-2022 Hubert Figuière
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

#[macro_use]
pub mod base;
pub mod capi;
pub mod toolkit;
pub mod utils;

pub use self::base::fractions::fraction_to_decimal;
pub use self::base::propertybag::PropertyBag;
pub use self::base::propertyvalue::PropertyValue;
pub use self::base::PropertySet;
pub use self::utils::exempi::{gps_coord_from_xmp, ExempiManager, NsDef, XmpMeta};

pub use self::base::date::*;

pub use self::toolkit::mimetype::MimeType;

use std::f64;

///
/// Init funtion because rexiv2 need one.
///
/// Make sure to call it after gtk::init()
///
pub fn init() {
    rexiv2::initialize().expect("Unable to initialize rexiv2");
}

// C++ bridge

use std::ffi::c_char;

use gdk_pixbuf_sys::GdkPixbuf;
use glib::translate::*;

use crate::base::date::Date;
use crate::toolkit::thumbnail::Thumbnail;
use crate::toolkit::Configuration;

fn make_config_path(file: &str) -> String {
    Configuration::make_config_path(file)
        .to_string_lossy()
        .into()
}

fn configuration_new(file: &str) -> cxx::SharedPtr<ffi::SharedConfiguration> {
    cxx::SharedPtr::new(ffi::SharedConfiguration {
        cfg: Box::new(Configuration::from_file(file)),
    })
}

fn exempi_manager_new() -> Box<ExempiManager> {
    Box::new(ExempiManager::new(None))
}

pub fn gps_coord_from_xmp_(value: &str) -> f64 {
    gps_coord_from_xmp(value).unwrap_or(f64::NAN)
}

pub fn fraction_to_decimal_(value: &str) -> f64 {
    fraction_to_decimal(value).unwrap_or(f64::NAN)
}

pub fn thumbnail_for_file(path: &str, w: i32, h: i32, orientation: i32) -> Box<Thumbnail> {
    Box::new(Thumbnail::thumbnail_file(path, w, h, orientation))
}

/// Create a %Thumbnail from a %GdkPixbuf
///
/// The resulting object must be freed by %fwk_toolkit_thumbnail_delete
///
/// # Safety
/// Dereference the pointer
unsafe fn thumbnail_from_pixbuf(pixbuf: *mut c_char) -> Box<Thumbnail> {
    let pixbuf: Option<gdk_pixbuf::Pixbuf> = from_glib_none(pixbuf as *mut GdkPixbuf);
    Box::new(Thumbnail::from(pixbuf))
}

fn thumbnail_to_pixbuf(self_: &Thumbnail) -> *mut c_char {
    let pixbuf: *mut GdkPixbuf = self_.make_pixbuf().to_glib_full();
    pixbuf as *mut c_char
}

#[cxx::bridge(namespace = "fwk")]
mod ffi {
    struct SharedConfiguration {
        cfg: Box<Configuration>,
    }

    extern "Rust" {
        type Configuration;

        #[cxx_name = "Configuration_new"]
        fn configuration_new(file: &str) -> SharedPtr<SharedConfiguration>;
        #[cxx_name = "Configuration_make_config_path"]
        fn make_config_path(file: &str) -> String;
        #[cxx_name = "hasKey"]
        fn has(&self, key: &str) -> bool;
        #[cxx_name = "getValue"]
        fn value(&self, key: &str, def: &str) -> String;
        #[cxx_name = "setValue"]
        fn set_value(&self, key: &str, value: &str);
    }

    extern "Rust" {
        type ExempiManager;

        #[cxx_name = "ExempiManager_new"]
        fn exempi_manager_new() -> Box<ExempiManager>;
    }

    extern "Rust" {
        #[cxx_name = "gps_coord_from_xmp"]
        fn gps_coord_from_xmp_(value: &str) -> f64;
        #[cxx_name = "fraction_to_decimal"]
        fn fraction_to_decimal_(value: &str) -> f64;
    }

    extern "Rust" {
        type Date;

        fn to_string(&self) -> String;
    }

    impl Box<Date> {}

    extern "Rust" {
        type Thumbnail;

        #[cxx_name = "Thumbnail_for_file"]
        fn thumbnail_for_file(path: &str, w: i32, h: i32, orientation: i32) -> Box<Thumbnail>;
        #[cxx_name = "Thumbnail_from_pixbuf"]
        unsafe fn thumbnail_from_pixbuf(pixbuf: *mut c_char) -> Box<Thumbnail>;
        #[cxx_name = "Thumbnail_to_pixbuf"]
        fn thumbnail_to_pixbuf(self_: &Thumbnail) -> *mut c_char;
    }
}
