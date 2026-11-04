//! Native integration needed by client-decorated Windows application windows.
use crate::deps::*;
use glib::translate::ToGlibPtr;
use std::ffi::c_void;

pub(crate) const GWL_STYLE: i32 = -16;
pub(crate) const WINDOW_ACTIONS: isize = 0x0008_0000 | 0x0002_0000 | 0x0001_0000;

#[link(name = "gtk-4")]
unsafe extern "C" {
    pub(crate) fn gdk_win32_surface_get_handle(surface: *mut gdk::ffi::GdkSurface) -> *mut c_void;
}

#[link(name = "user32")]
unsafe extern "system" {
    pub(crate) fn GetWindowLongPtrW(window: *mut c_void, index: i32) -> isize;
    fn SetWindowLongPtrW(window: *mut c_void, index: i32, value: isize) -> isize;
    fn SetWindowPos(
        window: *mut c_void,
        after: *mut c_void,
        x: i32,
        y: i32,
        width: i32,
        height: i32,
        flags: u32,
    ) -> i32;
    #[cfg(test)]
    pub(crate) fn SendMessageW(
        window: *mut c_void,
        message: u32,
        wparam: usize,
        lparam: isize,
    ) -> isize;
    #[cfg(test)]
    pub(crate) fn IsIconic(window: *mut c_void) -> i32;
}

pub(crate) fn handle(window: &impl IsA<gtk::Window>) -> Option<*mut c_void> {
    let surface = window.as_ref().surface()?;
    let handle = unsafe { gdk_win32_surface_get_handle(surface.to_glib_none().0) };
    (!handle.is_null()).then_some(handle)
}

pub(crate) fn enable_window_actions(window: &impl IsA<gtk::Window>) {
    let Some(handle) = handle(window) else { return };
    // GTK's client decorations omit these flags. Explorer checks them when
    // deciding whether a taskbar click may minimize the window. No native
    // caption is added, so Libadwaita keeps drawing the existing title bar.
    unsafe {
        let style = GetWindowLongPtrW(handle, GWL_STYLE);
        if style & WINDOW_ACTIONS != WINDOW_ACTIONS {
            SetWindowLongPtrW(handle, GWL_STYLE, style | WINDOW_ACTIONS);
            SetWindowPos(
                handle,
                std::ptr::null_mut(),
                0,
                0,
                0,
                0,
                0x0020 | 0x0001 | 0x0002 | 0x0004 | 0x0010,
            );
        }
    }
}
