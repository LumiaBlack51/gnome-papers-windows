#!/usr/bin/env python3
"""Run Windows GUI regressions on a private desktop without touching the user's UI.

Usage: python test-desktop.py LOGFILE EXECUTABLE [ARG ...]
The child and its descendants inherit the private desktop. It is never switched
to the input desktop. Output goes to LOGFILE; the child's exit code is preserved.
"""

import ctypes
from ctypes import wintypes as w
import os
from pathlib import Path
import subprocess
import sys
import uuid


class StartupInfo(ctypes.Structure):
    _fields_ = [("cb", w.DWORD), ("reserved", w.LPWSTR), ("desktop", w.LPWSTR),
                ("title", w.LPWSTR), ("x", w.DWORD), ("y", w.DWORD),
                ("width", w.DWORD), ("height", w.DWORD),
                ("xchars", w.DWORD), ("ychars", w.DWORD),
                ("fill", w.DWORD), ("flags", w.DWORD),
                ("show", w.WORD), ("reserved_size", w.WORD),
                ("reserved_data", ctypes.c_void_p),
                ("stdin", w.HANDLE), ("stdout", w.HANDLE), ("stderr", w.HANDLE)]


class ProcessInfo(ctypes.Structure):
    _fields_ = [("process", w.HANDLE), ("thread", w.HANDLE),
                ("pid", w.DWORD), ("tid", w.DWORD)]


def main():
    if os.name != "nt" or len(sys.argv) < 3:
        sys.exit("Usage (Windows only): test-desktop.py LOGFILE EXECUTABLE [ARG ...]")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.CreateDesktopW.argtypes = [w.LPCWSTR, w.LPCWSTR, ctypes.c_void_p,
                                    w.DWORD, w.DWORD, ctypes.c_void_p]
    user.CreateDesktopW.restype = w.HANDLE
    user.CloseDesktop.argtypes = [w.HANDLE]
    kernel.CreateProcessW.argtypes = [w.LPCWSTR, w.LPWSTR, ctypes.c_void_p,
                                     ctypes.c_void_p, w.BOOL, w.DWORD,
                                     ctypes.c_void_p, w.LPCWSTR,
                                     ctypes.POINTER(StartupInfo), ctypes.POINTER(ProcessInfo)]
    kernel.CreateProcessW.restype = w.BOOL
    kernel.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
    kernel.GetExitCodeProcess.argtypes = [w.HANDLE, ctypes.POINTER(w.DWORD)]
    kernel.CloseHandle.argtypes = [w.HANDLE]
    name = "papers-tests-" + uuid.uuid4().hex
    desktop = user.CreateDesktopW(name, None, None, 0, 0x10000000, None)
    if not desktop:
        raise ctypes.WinError(ctypes.get_last_error())
    info = ProcessInfo()
    try:
        import msvcrt
        log = Path(sys.argv[1]).resolve()
        log.parent.mkdir(parents=True, exist_ok=True)
        with log.open("wb") as output, open(os.devnull, "rb") as null:
            stdout = msvcrt.get_osfhandle(output.fileno())
            stdin = msvcrt.get_osfhandle(null.fileno())
            os.set_handle_inheritable(stdout, True)
            os.set_handle_inheritable(stdin, True)
            startup = StartupInfo(cb=ctypes.sizeof(StartupInfo), desktop=name,
                                  flags=0x100, stdin=stdin, stdout=stdout, stderr=stdout)
            command = ctypes.create_unicode_buffer(subprocess.list2cmdline(sys.argv[2:]))
            if not kernel.CreateProcessW(None, command, None, None, True, 0x08000000,
                                          None, None, ctypes.byref(startup), ctypes.byref(info)):
                raise ctypes.WinError(ctypes.get_last_error())
            kernel.CloseHandle(info.thread)
            print(f"Running on private desktop; log: {log}", flush=True)
            # A finite deadline prevents a failed GUI test from leaving a worker behind.
            if kernel.WaitForSingleObject(info.process, 3600_000) != 0:
                subprocess.run(["taskkill", "/PID", str(info.pid), "/T", "/F"],
                               capture_output=True, creationflags=0x08000000)
                raise TimeoutError("Background tests exceeded one hour")
            code = w.DWORD()
            if not kernel.GetExitCodeProcess(info.process, ctypes.byref(code)):
                raise ctypes.WinError(ctypes.get_last_error())
            return code.value
    finally:
        if info.process:
            kernel.CloseHandle(info.process)
        user.CloseDesktop(desktop)


if __name__ == "__main__":
    sys.exit(main())
