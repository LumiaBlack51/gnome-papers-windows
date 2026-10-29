# Windows maintenance release, 2026-10-29

Version: `51.beta.1-win-save.5`.

## Sources and behavior

- Adapt selected-text visibility from [our Linux build, 0e34afe](https://github.com/LumiaBlack51/papers-inplace-save/commit/0e34afe2602cce92bf88bfe43f889b8d80230d97). Initialize foreground alpha and use 30% opacity for region-only selections. The full Windows build already detects Poppler's transparent-selection API; retain that detection for both the backend and view library.
- Adapt [the Linux save-monitor guard, b450f0b](https://github.com/LumiaBlack51/papers-inplace-save/commit/b450f0b29c9ae20a190534317145fa0b1a3ef023). Suspend monitoring while writing the current document, discard queued reloads, and resume after either success or failure. Saving a copy to another path leaves the original file's monitoring active.
- On this Windows runtime, cancelling and recreating a native GIO watcher during a write caused heap corruption, reproduced separately with a small C program. Windows therefore polls modification time and size every 250ms. Suspending removes the polling source; resuming records a fresh baseline. It does not read the whole PDF on each tick. Non-Windows builds retain native GIO monitoring.
- Cherry-pick GNOME fixes `6d230c846`, `747b9402d`, `593459ef8`, `0003159db`, `94bf796ff`, and `3d4c052ed`: transparent signature backgrounds, signature sizing and previews, encoded local links, password accessibility, and internal GIR resources. Malformed percent escapes also retain their literal filename instead of panicking.

The base remains `51.beta.1`; this is a selective maintenance update, not a full 51.1 rebase. Preserve the existing Windows save-failure protection, PDF memory snapshots, annotation tools, enlarged UI, and fullscreen defaults.

## Validation

- Windows x64 release build succeeds.
- `pps-jobs-test`, `pps-annotations-test`, `pps-annotations-no-icons-test`, and `pps-selection-test` pass. The selection regression generates a disposable English/Chinese PDF and checks extraction, background rendering, drag updates, zoom, unfocused opacity, clearing selections, and foreground alpha initialization. Observed focused overlay alpha is 76/255 and unfocused alpha is 20/255.
- All five Rust regressions pass: local link decoding, transparent signature strokes, repeated monitor suspension with later external writes, cancellation of pending reload timeouts, and live GTK save behavior. The GTK check resolves compiled Ctrl+S/Ctrl+Shift+S shortcuts, completes three save/reopen cycles, retains the document/page/scroll position, checks failure and successful retry, and verifies a later external edit reloads.
- A packaged application was opened with a disposable English/Chinese PDF. Drag selection and Select All kept both scripts readable. Existing enlarged controls and fullscreen launch remained usable. The test window was closed afterward.

`build-aux/windows/build.ps1` runs the regression programs and checks annotations and selections again with PATH restricted to the bundled runtime before building the installer. Tests use generated or copied fixtures and memory-backed settings.

## Local cleanup

After publishing and verifying release attachments, remove project-local build caches, staging output, obsolete installers, and one-time diagnostics under `build/`. Keep the latest installer and checksum locally, along with reusable build scripts and regression sources in Git. Leave the MSYS2 toolchain, installed application, source checkout, and unrelated projects intact.
