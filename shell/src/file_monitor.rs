use super::*;
use std::cell::Cell;

mod imp {
    use super::*;

    #[derive(Properties, Default, Debug)]
    #[properties(wrapper_type = super::PpsFileMonitor)]
    pub struct PpsFileMonitor {
        #[property(construct_only, get)]
        pub(super) uri: RefCell<String>,
        pub(super) monitor: RefCell<Option<gio::FileMonitor>>,
        #[cfg(target_os = "windows")]
        pub(super) poll_id: RefCell<Option<glib::SourceId>>,
        pub(super) timeout_id: RefCell<Option<glib::SourceId>>,
        pub(super) suspend_count: Cell<u32>,
        pub(super) generation: Cell<u64>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for PpsFileMonitor {
        const NAME: &'static str = "PpsFileMonitor";
        type Type = super::PpsFileMonitor;
        type ParentType = glib::Object;
    }

    #[glib::derived_properties]
    impl ObjectImpl for PpsFileMonitor {
        fn signals() -> &'static [Signal] {
            static SIGNALS: OnceLock<Vec<Signal>> = OnceLock::new();
            SIGNALS.get_or_init(|| vec![Signal::builder("changed").run_last().action().build()])
        }

        fn constructed(&self) {
            self.start();
        }

        fn dispose(&self) {
            self.stop();
        }
    }

    impl PpsFileMonitor {
        pub(super) fn start(&self) {
            let file = gio::File::for_uri(self.uri.borrow().as_ref());
            #[cfg(target_os = "windows")]
            {
                // The Win32 GIO backend can corrupt its pending completion data
                // when a watcher is cancelled and recreated during a write.
                // Poll metadata instead; resuming starts from the saved version.
                let mut stamp = super::file_stamp(&file);
                let id = glib::timeout_add_local(
                    std::time::Duration::from_millis(250),
                    glib::clone!(
                        #[weak(rename_to = obj)]
                        self,
                        #[upgrade_or]
                        glib::ControlFlow::Break,
                        move || {
                            let current = super::file_stamp(&file);
                            if current != stamp {
                                let exists = current.is_some();
                                stamp = current;
                                if exists {
                                    obj.obj().emit_by_name::<()>("changed", &[]);
                                }
                            }
                            glib::ControlFlow::Continue
                        }
                    ),
                );
                self.poll_id.replace(Some(id));
            }
            #[cfg(not(target_os = "windows"))]
            match file.monitor_file(gio::FileMonitorFlags::NONE, gio::Cancellable::NONE) {
                Ok(monitor) => {
                    monitor.connect_changed(glib::clone!(
                        #[weak(rename_to = obj)]
                        self,
                        move |monitor, _, _, event| {
                            // Events already queued by a cancelled monitor must not reload
                            // a document after its own save has finished.
                            if monitor.is_cancelled() {
                                return;
                            }
                            match event {
                                gio::FileMonitorEvent::ChangesDoneHint => {
                                    obj.timeout_stop();
                                    obj.obj().emit_by_name::<()>("changed", &[]);
                                }
                                gio::FileMonitorEvent::Changed => obj.timeout_start(),
                                _ => (),
                            }
                        }
                    ));

                    self.monitor.replace(Some(monitor));
                }
                Err(e) => {
                    glib::g_warning!("", "{}", e.message());
                }
            }
        }

        pub(super) fn stop(&self) {
            self.timeout_stop();
            #[cfg(target_os = "windows")]
            if let Some(id) = self.poll_id.take() {
                id.remove();
            }
            if let Some(monitor) = self.monitor.take() {
                monitor.cancel();
            }
        }
        #[cfg_attr(target_os = "windows", allow(dead_code))]
        pub(super) fn timeout_start(&self) {
            self.timeout_stop();

            let id = glib::timeout_add_seconds_local_once(
                5,
                glib::clone!(
                    #[weak(rename_to = obj)]
                    self,
                    move || {
                        obj.timeout_id.take();
                        obj.obj().emit_by_name::<()>("changed", &[]);
                    }
                ),
            );

            self.timeout_id.replace(Some(id));
        }

        fn timeout_stop(&self) {
            if let Some(id) = self.timeout_id.take() {
                id.remove();
            }
        }
    }
}

#[cfg(target_os = "windows")]
fn file_stamp(file: &gio::File) -> Option<(u64, u32, i64)> {
    let info = file
        .query_info(
            "time::modified,time::modified-usec,standard::size",
            gio::FileQueryInfoFlags::NONE,
            gio::Cancellable::NONE,
        )
        .ok()?;
    Some((
        info.attribute_uint64("time::modified"),
        info.attribute_uint32("time::modified-usec"),
        info.size(),
    ))
}

glib::wrapper! {
    pub struct PpsFileMonitor(ObjectSubclass<imp::PpsFileMonitor>);
}

impl PpsFileMonitor {
    pub fn new(uri: &str) -> Self {
        glib::Object::builder().property("uri", uri).build()
    }

    pub fn is_suspended(&self) -> bool {
        self.imp().suspend_count.get() > 0
    }

    pub fn generation(&self) -> u64 {
        self.imp().generation.get()
    }

    pub fn suspend(&self) -> FileMonitorGuard {
        let imp = self.imp();
        imp.suspend_count.set(imp.suspend_count.get() + 1);
        imp.generation.set(imp.generation.get().wrapping_add(1));
        imp.stop();
        FileMonitorGuard(self.clone())
    }
}

// Keep the monitor suspended until the save callback is disconnected, including
// error paths. A fresh monitor cannot receive notifications from the old write.
pub struct FileMonitorGuard(PpsFileMonitor);

impl Drop for FileMonitorGuard {
    fn drop(&mut self) {
        let imp = self.0.imp();
        let count = imp.suspend_count.get() - 1;
        imp.suspend_count.set(count);
        if count == 0 {
            imp.start();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::rc::Rc;
    use std::time::{Duration, Instant};

    fn pump_for(duration: Duration) {
        let context = glib::MainContext::default();
        let until = Instant::now() + duration;
        while Instant::now() < until {
            while context.pending() {
                context.iteration(false);
            }
            std::thread::sleep(Duration::from_millis(5));
        }
    }

    #[test]
    fn own_write_and_queued_events_are_ignored_but_external_writes_resume() {
        let context = glib::MainContext::default();
        let _context = context.acquire().unwrap();
        let path = std::env::temp_dir().join(format!("papers-monitor-{}", std::process::id()));
        std::fs::write(&path, b"original").unwrap();
        let file = gio::File::for_path(&path);
        let monitor = PpsFileMonitor::new(&file.uri());
        let changes = Rc::new(Cell::new(0));
        monitor.connect_closure(
            "changed",
            false,
            glib::closure_local!(
                #[strong]
                changes,
                move |_: PpsFileMonitor| changes.set(changes.get() + 1)
            ),
        );

        for _ in 0..3 {
            #[cfg(not(target_os = "windows"))]
            let old = monitor.imp().monitor.borrow().clone().unwrap();
            let generation = monitor.generation();
            let guard = monitor.suspend();
            let nested_guard = monitor.suspend();
            #[cfg(not(target_os = "windows"))]
            assert!(old.is_cancelled());
            #[cfg(target_os = "windows")]
            assert!(monitor.imp().poll_id.borrow().is_none());
            std::fs::write(&path, b"own save").unwrap();
            pump_for(Duration::from_millis(300));
            assert_eq!(changes.get(), 0);
            drop(guard);
            assert!(monitor.is_suspended());
            drop(nested_guard);
            assert!(!monitor.is_suspended());
            assert_ne!(monitor.generation(), generation);

            // Deliver a notification from the old monitor after save completion.
            #[cfg(not(target_os = "windows"))]
            old.emit_by_name::<()>(
                "changed",
                &[
                    &file,
                    &Option::<gio::File>::None,
                    &gio::FileMonitorEvent::ChangesDoneHint,
                ],
            );
            pump_for(Duration::from_millis(300));
            assert_eq!(changes.get(), 0);
        }

        std::fs::write(&path, b"external update after save").unwrap();
        // Win32 may report Changed without ChangesDoneHint, using the existing
        // five-second debounce instead of the immediate completion event.
        let deadline = Instant::now() + Duration::from_secs(8);
        while changes.get() == 0 && Instant::now() < deadline {
            pump_for(Duration::from_millis(20));
        }
        assert!(changes.get() > 0, "external updates must still be detected");
        monitor.imp().stop();
        std::fs::remove_file(path).unwrap();
    }

    #[test]
    fn suspending_cancels_the_pending_reload_timeout() {
        let context = glib::MainContext::default();
        let _context = context.acquire().unwrap();
        let path = std::env::temp_dir().join(format!("papers-timeout-{}", std::process::id()));
        std::fs::write(&path, b"original").unwrap();
        let file = gio::File::for_path(&path);
        let monitor = PpsFileMonitor::new(&file.uri());
        let changes = Rc::new(Cell::new(0));
        monitor.connect_closure(
            "changed",
            false,
            glib::closure_local!(
                #[strong]
                changes,
                move |_: PpsFileMonitor| changes.set(changes.get() + 1)
            ),
        );
        monitor.imp().timeout_start();
        assert!(monitor.imp().timeout_id.borrow().is_some());
        drop(monitor.suspend());
        assert!(monitor.imp().timeout_id.borrow().is_none());
        pump_for(Duration::from_secs(6));
        assert_eq!(changes.get(), 0);
        monitor.imp().stop();
        std::fs::remove_file(path).unwrap();
    }
}
