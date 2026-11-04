# Windows maintenance release, 2026-11-04

Version: `51.beta.1-win-save.7`.

- Restore the native system-menu, minimize and maximize style bits omitted by
  client decorations, so Explorer can minimize the window from its taskbar button.
  Keep the existing Libadwaita title bar. Apply startup maximization once instead
  of forcing it again on every map; opening a document still maximizes it.
- In the document region, decline GTK's DirectManipulation touchpad contact so
  Windows supplies compatible wheel input. Intercept Ctrl+wheel before GTK routes
  it, preserving signed fractional deltas and the pointer's position as the zoom
  center. Ordinary scrolling, sidebar input and other windows keep their routing.
  See Microsoft's [precision touchpad input documentation](https://learn.microsoft.com/en-us/windows/win32/input-precisiontouchpad/precision-touchpad-portal)
  and [wheel message contract](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel).
- Correct the GTK pinch callback to divide cumulative scales rather than subtract
  them, and ignore invalid scales or events without a loaded document.
- Run builds and GUI regressions on a private Windows desktop. Never switch to
  it or inject physical input. Retain reusable test and cleanup tooling in Git;
  delete disposable diagnostics and build output after release verification.

## Validation

The Windows x64 release build and all five Meson groups pass: jobs, annotations,
annotations without cursor resources, selection rendering, and Cargo regressions.
The annotation and selection checks also run against the bundled runtime with
the build toolchain excluded from PATH.

The shell regression checks native style flags, `SC_MINIMIZE`/`SC_RESTORE`, a
stable minimized state, remapping a restored window, document maximization and
fullscreen transitions. With a loaded PDF it sends native Ctrl+wheel messages:
ten +12 deltas must produce exactly one 1.2× step, -120 must reverse it, and an
unmodified wheel message must not zoom. It also exercises the compiled GTK
pinch callback with cumulative scales 1.2, 1.5 and 1.0. Existing save, annotation,
selection and external-file-change regressions remain in the build.

These are isolated native message and application regressions. They do not
simulate a physical touchpad or interact with the user's live taskbar.
