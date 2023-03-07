/*
 * niepce - library/thumbnail_cache.rs
 *
 * Copyright (C) 2020-2023 Hubert Figuière
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

use std::cmp;
use std::fs;
use std::path::{Path, PathBuf};

use crate::db;
use crate::db::libfile::{FileStatus, LibFile};
use crate::library::notification;
use crate::library::notification::LibNotification::{
    FileStatusChanged, ImageRendered, ThumbnailLoaded,
};
use crate::library::notification::{FileStatusChange, LcChannel};
use crate::library::previewer::{Cache, RenderMsg, RenderSender, RenderingParams, RenderingType};
use npc_fwk::base::Size;
use npc_fwk::toolkit;
use npc_fwk::toolkit::thumbnail::Thumbnail;
use npc_fwk::toolkit::ImageBitmap;
use npc_fwk::{dbg_out, err_out, on_err_out};

/// Previewing task
struct Task {
    /// Type of rendering task
    type_: RenderingType,
    /// Requested dimensions
    dimensions: Size,
    /// File to generate thumbnail for.
    file: LibFile,
    ///
    processor: Option<RenderSender>,
}

impl Task {
    /// Create a new thumbnailing Task
    pub fn new_thumbnail(file: LibFile, w: u32, h: u32) -> Self {
        Task {
            type_: RenderingType::Thumbnail,
            file,
            dimensions: Size { w, h },
            processor: None,
        }
    }

    pub fn new_rendering(file: LibFile, processor: Option<RenderSender>) -> Self {
        Task {
            type_: RenderingType::Preview,
            file,
            dimensions: Size::default(),
            processor,
        }
    }
}

/// Check the file status (ie is it still present?) and report.
fn check_file_status(id: db::LibraryId, path: &Path, sender: &LcChannel) {
    if !path.is_file() {
        dbg_out!("file doesn't exist");
        if let Err(err) =
            toolkit::thread_context().block_on(sender.send(FileStatusChanged(FileStatusChange {
                id,
                status: FileStatus::Missing,
            })))
        {
            err_out!("Sending file status change notification failed: {}", err);
        }
    }
}

fn get_preview(
    cache: &Cache,
    task: &Task,
    rendering: &RenderingParams,
    sender: &LcChannel,
) -> Option<ImageBitmap> {
    let libfile = &task.file;
    let filename = libfile.path().to_string_lossy();
    let dimensions = rendering.dimensions;
    let dimension = cmp::max(dimensions.w, dimensions.h);
    // true if we found a cache entry but no file.

    let dest = cache.path_for_thumbnail(libfile.path(), libfile.id(), dimension)?;
    if dest.exists() {
        dbg_out!("found {filename} in the cache fs");
        cache.hit(&filename, dimension);
        return ImageBitmap::from_file(dest).ok();
    }

    if let Ok(cache_item) = cache.get(&filename, dimension) {
        // cache hit
        dbg_out!("thumbnail for {:?} is cached!", filename);
        if cache_item.target.exists() {
            return ImageBitmap::from_file(cache_item.target).ok();
        } else {
            dbg_out!("File is missing");
        }
    }

    dbg_out!("creating preview for {:?}", filename);
    if let Some(cached_dir) = dest.parent() {
        if let Err(err) = fs::create_dir_all(cached_dir) {
            err_out!("Coudln't create directories for {:?}: {}", dest, err);
        }
    }

    // Run the pipeline
    if let Some(processor) = &task.processor {
        let sender = sender.clone();
        let rendering = rendering.clone();
        let cache_sender = cache.sender();
        let filename = filename.to_string();
        on_err_out!(processor.send(RenderMsg::GetBitmap(Box::new(move |pix| {
            if let Err(err) =
                toolkit::thread_context().block_on(sender.send(ImageRendered(pix.clone())))
            {
                err_out!("Sending image rendered notification failed: {}", err);
            }
            on_err_out!(pix.save_png(&dest));
            on_err_out!(cache_sender.send(super::previewer::DbMessage::Put(
                filename.clone(),
                dimension,
                rendering.clone(),
                dest.to_string_lossy().to_string(),
            )));
        }))));
    } else {
        err_out!("no processor");
    }
    None
}

fn get_thumbnail(
    cache: &Cache,
    libfile: &LibFile,
    rendering: &RenderingParams,
) -> Option<Thumbnail> {
    let filename = libfile.path().to_string_lossy();
    let dimensions = rendering.dimensions;
    let dimension = cmp::max(dimensions.w, dimensions.h);
    // true if we found a cache entry but no file.
    let mut is_missing = false;

    let dest = cache.path_for_thumbnail(libfile.path(), libfile.id(), dimension)?;
    if dest.exists() {
        cache.hit(&filename, dimension);
        return gdk_pixbuf::Pixbuf::from_file(dest)
            .ok()
            .map(Thumbnail::from);
    }

    if let Ok(cache_item) = cache.get(&filename, dimension) {
        // cache hit
        dbg_out!("thumbnail for {:?} is cached!", filename);
        if cache_item.target.exists() {
            return gdk_pixbuf::Pixbuf::from_file(cache_item.target)
                .ok()
                .map(Thumbnail::from);
        } else {
            dbg_out!("File is missing");
            is_missing = true;
        }
    }

    dbg_out!("creating thumbnail for {:?}", filename);
    if let Some(cached_dir) = dest.parent() {
        if let Err(err) = fs::create_dir_all(cached_dir) {
            err_out!("Coudln't create directories for {:?}: {}", dest, err);
        }
    }

    let thumbnail = Thumbnail::thumbnail_file(
        libfile.path(),
        dimensions.w,
        dimensions.h,
        libfile.orientation(),
    );
    if let Some(ref thumbnail) = thumbnail {
        thumbnail.save(&dest, "png");
        if !is_missing {
            // We don't need to try to add it back
            // XXX maybe should we update the create date if it was recreated?
            cache.put(
                &filename,
                dimension,
                rendering.clone(),
                &dest.to_string_lossy(),
            );
        }
    } else {
        dbg_out!("couldn't get the preview for {:?}", filename);
    }
    thumbnail
}

pub struct ThumbnailCache {
    /// Queue to send task
    queue_sender: std::sync::mpsc::Sender<Vec<Task>>,
}

impl ThumbnailCache {
    pub fn new(dir: &Path, sender: LcChannel) -> Self {
        let (queue_sender, queue) = std::sync::mpsc::channel();
        let cache_dir = PathBuf::from(dir);
        on_err_out!(std::thread::Builder::new()
            .name("thumbnail cache".to_string())
            .spawn(move || {
                Self::main(cache_dir, queue, sender);
            }));

        Self { queue_sender }
    }

    fn execute(task: &Task, cache: &Cache, sender: &LcChannel) {
        let libfile = &task.file;
        let id = libfile.id();
        let path = libfile.path();
        let dimensions = task.dimensions;
        // We shall report if the file is missing.
        check_file_status(id, path, sender);

        match task.type_ {
            RenderingType::Preview => {
                let rendering = RenderingParams::new_preview(id, dimensions);
                if let Some(pix) = get_preview(cache, task, &rendering, sender) {
                    dbg_out!("Got the preview from the cache");
                    if let Err(err) =
                        toolkit::thread_context().block_on(sender.send(ImageRendered(pix)))
                    {
                        err_out!("Sending image rendered notification failed: {}", err);
                    }
                }
            }
            RenderingType::Thumbnail => {
                // XXX this should take into account the size.
                let rendering = RenderingParams::new_thumbnail(id, dimensions);
                if let Some(pix) = get_thumbnail(cache, libfile, &rendering) {
                    // notify the thumbnail
                    if let Err(err) = toolkit::thread_context().block_on(sender.send(
                        ThumbnailLoaded(notification::Thumbnail {
                            id,
                            width: pix.get_width(),
                            height: pix.get_height(),
                            pix,
                        }),
                    )) {
                        err_out!("Sending thumbnail notification failed: {}", err);
                    }
                } else {
                    err_out!("Failed to get thumbnail for {:?}", path);
                }
            }
        }
    }

    fn main(cache_dir: PathBuf, queue: std::sync::mpsc::Receiver<Vec<Task>>, sender: LcChannel) {
        let cache = Cache::new(cache_dir);
        cache.initialize();
        dbg_out!("Cache database ready");
        while let Ok(tasks) = queue.recv() {
            dbg_out!("Parallel thumbnailing of {} files", tasks.len());
            tasks.iter().for_each(|task| {
                Self::execute(task, &cache, &sender);
            })
        }
        dbg_out!("thumbnail cache thread terminating");
    }

    /// Request a render.
    pub fn request_render(&self, file: LibFile, processor: Option<RenderSender>) {
        on_err_out!(self
            .queue_sender
            .send(vec![Task::new_rendering(file, processor)]));
    }

    /// Request thumbnails.
    pub fn request(&self, fl: &[LibFile]) {
        on_err_out!(self.queue_sender.send(
            fl.iter()
                .map(|f| Task::new_thumbnail(f.clone(), 160, 160))
                .collect()
        ));
    }
}
