---
name: uniqlogger-build
description: How to build the UniqLogger library with qmake for macOS, Linux, Windows, iOS (also as an XCFramework) and Android, which options in config.pri turn writers on, where the output lands, and how to make a package. Use when you must compile the library, produce a binary for another library to link, or debug a build or dependency problem.
---

# Building UniqLogger

The build is qmake based: `lib/uniqlogger.pro`. The public way to build is `lib/00-compile` (clone the dependencies,
run qmake, run make). Everything below is that procedure plus the details an agent needs.

## Local environment
Machine specific values. The defaults are the maintainer's; ask the user or look at the machine when they differ.
- `QT_ROOT`: the Qt version folder, default `~/Qt/6.8.6`. It holds one subfolder per platform (`macos`, `ios`,
  `android_arm64_v8a`, `android_x86_64`...), each with its own `bin/qmake`.
- `UNQL_SRC`: the checkout you build, default the current worktree. Build in a folder outside the source
  (`$UNQL_SRC/lib/build/<kit>` is git-ignored), never inside `lib/src`.

## 1. Dependencies
`lib/uniqlogger.pro` includes three Qt libraries from GitHub (`netresultsit`): filecompressor, qt-threadpool and
(only with the database switch) qt-dbhandler. `lib/deps.json` lists each one with its folder under `lib/src/ext` and
the tag to use. Without `lib/depspath.pri`, qmake looks in `lib/src/ext/<name>/src`.
- Public way: `git clone --branch <tag from deps.json> <url from deps.json> lib/src/ext/<folder>` for each entry
  (`00-compile` clones the default branch; use the tag so the build is reproducible).
- Dependencies elsewhere: copy `lib/depspath.pri.sample` to `lib/depspath.pri` and set `FILECOMPRESSOR_ROOT`,
  `THREADPOOL_ROOT`, `DBH_ROOT` (each points to the `src` folder of the dependency).
- An internal dependency tool can fill `lib/src/ext` from `lib/deps.json`; it is not needed.
- qmake stops with "Missing nrFileCompressor.pri" (or nrThreadPool.pri, nrDbHandler.pri) when a path is wrong.
- `lib/src/ext`, `lib/config.pri` and `lib/depspath.pri` are git-ignored: never commit them.

## 2. Options (`lib/config.pri`)
Copy `lib/config.pri.sample` to `lib/config.pri` and uncomment what you need. Without the file the library is built
with no network and no database support.
| Switch | Effect |
|---|---|
| `DEFINES += ENABLE_UNQL_NETLOG` | `RemoteWriter` and `RSyslogWriter`, adds `QT += network` |
| `DEFINES += ENABLE_UNQL_DBLOG` | `DbWriter` and `UnqlDbHandler`, adds `QT += sql` and the dbhandler dependency |
| `DEFINES += ENABLE_UNQL_ANDROIDLOG` | `AndroidWriter`; defined automatically by the `.pro` on Android |
| `DEFINES += ENABLE_UNQL_DBG` | turns `ULDBG` on (very verbose) and the threadpool debug output |
| `IDE = VS` | generate a Visual Studio project on Windows |
| `IOS_NOT_MOVE = true` | on iOS, do not copy the binary to `last_build` and the final folder |
Build with the switches the consumer needs, and test the combinations a change touches (at least none and all).

## 3. Build
From a build folder (`$UNQL_SRC/lib/build/<kit>`):
```
$QT_ROOT/macos/bin/qmake -o Makefile ../../uniqlogger.pro -spec macx-clang CONFIG+=release
make -j8 release        # or: make debug
```
- macOS and Linux: use the `bin/qmake` of the desktop Qt. On Linux the spec is the Qt default.
- macOS with Qt 6 on an Apple Silicon host builds arm64 and x86_64 (universal); on an Intel host, x86_64 only.
- Windows: Visual Studio 2017 to 2022 (the compiler is detected from `QMAKE_MSC_VER`), `nmake` or `jom` in a
  Visual Studio command prompt (not verified on this branch).
- Debug and release are separate targets of one Makefile; `make` without a target builds what `CONFIG` says.
- Build output of a debug library has a suffix (`_debug` on macOS and Android, `_d` on Linux, `d1` on Windows)
  so both can sit in `last_build`.
- Mobile: read `platforms.md` (iOS, XCFramework, Android).
- Do not run a build in a checkout someone else is using: its `last_build` is what other libraries link.

## 4. Where the output lands
- The binary is created in `<build folder>/<debug|release>/bin`.
- A post-link step copies it to `lib/last_build/` and to `lib/<debug|release>/<version>_<arch>_<compiler>_qt-<qt version>/`
  (the source tree, git-ignored), and copies the installed headers to `lib/last_build/include/`. Other libraries
  consume `last_build` or that versioned folder.
- On macOS the install name is set to `@rpath/libUniqLogger[_debug].<major>.dylib`.
- The installed headers are the `INCLUDE_HEADERS` of `lib/uniqlogger.pro`; writer headers (`FileWriter.h`,
  `ConsoleWriter.h`...) are not installed.
- Documentation: `lib/Doxyfile` reads `src` and writes into `last_build` (run `doxygen Doxyfile` in `lib`, not
  verified).

## 5. Packages
`lib/create_pkg.py` run in `lib` packs `last_build` as `uniqlogger-<version>-<platform>-r<git hash>.tgz` (a zip on
Windows, which also needs a `vs_used.txt` file with the compiler banner). `UNQL_OS` overrides the platform name. The
script is written for Python 2 (a `print` statement): not verified with Python 3. Publishing a package or a tag
is outside the repository: ask first (`gate.outside` in `AGENTS.md`).

## 6. Typical failures
- "Missing nrFileCompressor.pri": the dependency folder or `depspath.pri` is wrong (section 1).
- iOS, Qt 6.8, Apple Clang: `implicitly declaring library function '__yield'`: `lib/qt_yield_workaround.pri`
  force-includes `lib/src/qt_arm_acle_workaround.h`; do not remove it, it switches itself off when Qt is fixed.
- Android: the `.pro` stops with Qt older than 6.8.4 (16 KB page alignment) and with the armeabi and x86 ABIs.
- A test or consumer cannot find the library: it is not in `lib/last_build`, or the debug suffix is wrong.
