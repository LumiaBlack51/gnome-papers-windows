# Local maintenance

- Keep builds and GUI regressions in the background. Use `build-aux/windows/build.ps1`,
  which runs on a private Windows desktop. Never switch desktops, move the pointer,
  send physical keyboard input, or open visible diagnostic windows during maintenance.
- After publishing a release, verify its uploaded installer and SHA-256 checksum,
  then run `build-aux/windows/clean.ps1 -KeepVersion <version>` to remove local build
  caches, staging directories, obsolete installers, and one-time diagnostics.
- Keep the latest installer and checksum, tracked source, reusable build/test scripts,
  MSYS2 toolchain, installed application, and unrelated projects. Put temporary
  diagnostics inside `build/` so cleanup removes them. Never commit one-time scripts.
- These cleanup rules apply to future maintenance and release tasks as well.
