/*
 * niepce - lib.rs
 *
 * Copyright (C) 2017-2023 Hubert Figuière
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
extern crate gtk_macros;
extern crate libadwaita as adw;

mod import;
pub mod modules;
pub mod niepce;
mod notification_center;

use std::sync::Once;

fn niepce_init() {
    static START: Once = Once::new();

    START.call_once(|| {
        gtk4::init().unwrap();
        adw::init().unwrap();
        npc_fwk::init();
    });
}

pub use notification_center::NotificationCenter;

// cxx bindings
use niepce::ui::cxx::*;
use niepce::ui::image_list_store::ImageListStoreWrap;
use niepce::ui::imagetoolbar::image_toolbar_new;
use niepce::ui::metadata_pane_controller::get_format;
use niepce::ui::niepce_window::{niepce_window_new, NiepceWindowWrapper};
use niepce::ui::ImageGridView;
use niepce::ui::{ImageListStore, SelectionController};
use npc_fwk::toolkit;

#[cxx::bridge(namespace = "npc")]
pub mod ffi {
    #[namespace = ""]
    unsafe extern "C++" {
        type GMenu;
        type GtkApplication;
        type GtkBox;
        type GtkGridView;
        type GtkPopoverMenu;
        type GtkSingleSelection;
        type GtkWidget;
        type GtkWindow;
    }

    extern "Rust" {
        fn niepce_init();
    }

    #[namespace = "fwk"]
    extern "C++" {
        include!("fwk/cxx_colour_bindings.hpp");
        include!("fwk/cxx_widgets_bindings.hpp");

        type RgbColour = npc_fwk::base::rgbcolour::RgbColour;
        type MetadataSectionFormat = crate::toolkit::widgets::MetadataSectionFormat;
    }

    #[namespace = "eng"]
    extern "C++" {
        include!("fwk/cxx_prelude.hpp");
        type Label = npc_engine::db::Label;
        type LibFile = npc_engine::db::LibFile;
        type ThumbnailCache = npc_engine::ThumbnailCache;
        type LibNotification = npc_engine::library::notification::LibNotification;
        type UIDataProvider = npc_engine::libraryclient::UIDataProvider;
        type LibraryClientWrapper = npc_engine::libraryclient::LibraryClientWrapper;
        type LibraryClientHost = npc_engine::libraryclient::LibraryClientHost;
    }

    extern "Rust" {
        fn get_format() -> &'static [MetadataSectionFormat];
    }

    #[namespace = "ui"]
    unsafe extern "C++" {
        include!("niepce/ui/metadatapanecontroller.hpp");
        type MetaDataPaneController;

        fn metadata_pane_controller_new() -> SharedPtr<MetaDataPaneController>;
        fn build_widget(&self) -> *mut GtkWidget;
    }

    extern "Rust" {
        type NiepceWindowWrapper;

        unsafe fn niepce_window_new(app: *mut GtkApplication) -> Box<NiepceWindowWrapper>;
        fn on_ready(&self);
        fn on_open_catalog(&self);
        fn widget(&self) -> *mut GtkWidget;
        fn window(&self) -> *mut GtkWindow;
        fn menu(&self) -> *mut GMenu;
    }

    #[namespace = "ui"]
    extern "Rust" {
        type ImageListStore;

        fn clear_content(&self);
        fn gobj(&self) -> *mut GtkSingleSelection;
        fn get_file_(&self, id: i64) -> *mut LibFile;
        fn get_file_id_at_pos(&self, pos: u32) -> i64;
        fn get_pos_from_id_(&self, id: i64) -> u32;
    }

    #[namespace = "ui"]
    extern "Rust" {
        type ImageListStoreWrap;

        fn unwrap_ref(&self) -> &ImageListStore;
        #[cxx_name = "clone"]
        fn clone_(&self) -> Box<ImageListStoreWrap>;
    }

    #[namespace = "fwk"]
    extern "C++" {
        include!("fwk/cxx_widgets_bindings.hpp");

        type WrappedPropertyBag = crate::toolkit::widgets::WrappedPropertyBag;
    }

    #[namespace = "ui"]
    extern "Rust" {
        fn image_toolbar_new() -> *mut GtkBox;
    }

    extern "Rust" {
        type ImageGridView;

        unsafe fn npc_image_grid_view_new(
            store: *mut GtkSingleSelection,
            context_menu: *mut GtkPopoverMenu,
            libclient_host: &LibraryClientHost,
        ) -> Box<ImageGridView>;
        fn get_grid_view(&self) -> *mut GtkGridView;
        fn add_rating_listener(&self, listener: UniquePtr<RatingClickListener>);
    }

    #[namespace = "ui"]
    extern "Rust" {
        type SelectionController;

        #[cxx_name = "get_list_store"]
        fn list_store(&self) -> &ImageListStoreWrap;

        fn set_rating_of(&self, id: i64, rating: i32);
        fn set_properties(&self, props: &WrappedPropertyBag, old: &WrappedPropertyBag);
    }

    unsafe extern "C++" {
        include!("niepce/ui/rating_click_listener.hpp");
        type RatingClickListener;

        fn call(&self, id: i64, rating: i32);
    }

    #[namespace = "ui"]
    unsafe extern "C++" {
        include!("niepce/ui/gridviewmodule.hpp");
        type GridViewModule;

        /// # Safety
        /// Dereference a pointer
        unsafe fn grid_view_module_new(
            selection_controller: &SelectionController,
            menu: *const GMenu,
            libclient_host: &LibraryClientHost,
        ) -> SharedPtr<GridViewModule>;
        // call buildWidget(). But it's mutable.
        fn build_widget(&self) -> *const GtkWidget;
        fn on_lib_notification(&self, ln: &LibNotification, client: &LibraryClientWrapper);
        fn display_none(&self);
        #[cxx_name = "cxx_image_list"]
        fn image_list(&self) -> *const GtkGridView;
    }

    #[namespace = "mapm"]
    unsafe extern "C++" {
        include!("niepce/modules/map/mapmodule.hpp");
        type MapModule;

        fn map_module_new() -> SharedPtr<MapModule>;
        // call buildWidget(). But it's mutable.
        fn build_widget(&self) -> *const GtkWidget;
        fn on_lib_notification(&self, ln: &LibNotification);
        fn set_active(&self, active: bool);
    }

    #[namespace = "dr"]
    unsafe extern "C++" {
        include!("niepce/modules/darkroom/darkroommodule.hpp");
        type DarkroomModule;

        fn darkroom_module_new() -> SharedPtr<DarkroomModule>;
        // call buildWidget(). But it's mutable.
        fn build_widget(&self) -> *const GtkWidget;
        fn set_active(&self, active: bool);
        /// # Safety
        /// Dereference a pointer
        unsafe fn set_image(&self, file: *mut LibFile);
    }

    #[namespace = "ui"]
    unsafe extern "C++" {
        include!("niepce/ui/dialogs/editlabels.hpp");
        type EditLabels;

        fn edit_labels_new(libclient: &LibraryClientHost) -> SharedPtr<EditLabels>;
        /// # Safety
        /// Dereference a pointer
        unsafe fn run_modal(
            &self,
            parent: *mut GtkWindow,
            on_ok: fn(SharedPtr<EditLabels>, i32),
            this_: SharedPtr<EditLabels>,
        );
    }
}
