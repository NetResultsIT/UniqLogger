---
name: uniqlogger-dev-workflow
description: UniqLogger specifics of the development workflow - the classes and where to start, how tests are designed here, the gate and review additions, the design rules, the public API and enum values other libraries cast, the writer threading model and lock order, the ignored files. Extends the shared skill dev-workflow; read that first. Use before planning, implementing or reviewing a change in this repository.
---

# UniqLogger development workflow

Extends the shared skill `dev-workflow`: read it first, it has the phases, the worker brief, the review checklist,
the report and the routing table. This skill adds what is specific to UniqLogger; it may add steps or make one
stricter, never relax one. Building is in `uniqlogger-build`, running the tests in `uniqlogger-run-tests`.

## 1. Orientation
The classes: `UniqLogger` (instances, writer registry, `NRThreadPool`), `Logger` (the user API, buffers messages
per thread), `LogWriter` and its subclasses (`FileWriter`, `ConsoleWriter`, `DummyWriter`, and with switches
`RemoteWriter`, `RSyslogWriter`, `DbWriter`, `AndroidWriter`), `WriterConfig`, `LogMessage`, `TimeUtils`,
`SysLogMessageFactory`.

## 2. Tests here
Which program (`uniqlogger-run-tests`, section 5): `TestRegressionCoverage` for behaviour of the public API,
`TestFileWriterRotationScenarios` for rotation, `lib/ut` for one pure class. Cases, on top of those of
`dev-workflow`: wrong arguments and wrong state with the expected `ok` code, two loggers on one writer, a logger used
from two threads, and which `config.pri` switches the test needs.

## 3. Gate additions
- Say which consumers are affected (other libraries cast the enums and link the library) and the version number you
  would set.
- For a public API change also say whether the binary interface changes (size or layout of an exported class, a
  virtual, a changed signature).

## 4. Additions to the worker brief
- Files not to touch: public enum values, `lib/config.pri`, `lib/depspath.pri`, `lib/src/ext`, the build files unless
  the plan says so.
- The baseline: what the tests give before the change, with the `Totals:` lines; the worker reports the `Totals:`
  line of every program and what could not be built or run (a platform, a switch).

## 5. Review items
- [ ] `ULOG_LIB_API` still on every exported class; no new public header without the user's consent (it must be added
      to `INCLUDE_HEADERS` and to `build_ios_xcframework.sh`);
- [ ] optional code guarded by its switch, built with and without the switches you can run;
- [ ] the threading rules below;
- [ ] Doxygen comments, `README.md` and (when the version changes) `lib/Doxyfile` updated;
- [ ] new files have the copyright header like their neighbours;
- [ ] no internal host names, URLs or personal paths in anything added (the repository is public);
- [ ] platforms not checked (Windows, mobile) are named in the report.

## 6. Design rules of this repository
- `UniqLogger.cpp` (about 1000 lines) and `FileWriter.cpp` (about 850 lines) are already large. Put new rotation,
  naming or compression logic in a small helper class, as `TimeUtils` does, instead of growing them.
- A new kind of destination is a new `LogWriter` subclass plus a `create*Logger` / `get*Writer` pair in
  `UniqLogger` that reuses the registry and the `ok` convention; do not special-case a writer in `Logger`.
- Configuration is `WriterConfig`; do not add a second configuration channel. A new field must keep the existing
  fields' order and meaning, extend `operator==`, `toString()` and the sanity check, and default to the old behavior.
- Use `ULDBG` for debug output and `UNQL::ErrorType` for errors.
- Prefer a Qt container and Qt string type in headers; do not expose a dependency type (`NRThreadPool`, the
  filecompressor classes) in an installed header.

## 7. Public API and compatibility
- The installed headers are the `INCLUDE_HEADERS` of `lib/uniqlogger.pro`. Exported classes carry `ULOG_LIB_API`
  (a `dllexport` / `dllimport` macro on Windows only, so a missing macro shows up only on Windows). `LogWriter` is not
  exported, which is why some tests compile it in on Windows.
- Never change the value of an existing enumerator and never insert one in the middle (enumerators without a value
  are numbered by position). Add new ones at the end, or with a new explicit value.
- The version bump comes after the change, in its own commit "version bump to x.y.z" (`git log --grep "version
  bump"`); releases are tagged with the plain version. `lib/Doxyfile` `PROJECT_NUMBER` and
  `lib/ci/sonar-project.properties` hold older copies; do not treat them as the source of the version.
- The macOS install name and the Windows DLL name contain the major version: a major bump breaks consumers' link
  step by design.

## 8. Threading model (what the code shows)
- `UniqLogger::instance(name, nthreads)` returns one object per name for the process, guarded by a static mutex.
  The thread count is fixed by the first call for a name; a later call with another count only logs a warning.
- Each instance owns an `NRThreadPool`. A writer is created by `get*Writer`, registered in the instance's map with a
  use count, and handed to the pool with `runObject`: it lives on a pool thread, and its flush `QTimer` (interval
  `writerFlushSecs`, default 5) runs there. The timer is started and stopped with `QMetaObject::invokeMethod` by
  the slot names `priv_startLogTimer` and `priv_stopLogTimer`: renaming them breaks it silently.
- Writers are shared: `getFileWriter` returns the existing writer when the base file name matches, so two loggers on
  one path share one writer and one `WriterConfig`. A conflicting config is accepted only when no other logger uses
  the writer; otherwise `ok` is `UnqlErrorWriterConfigIncompatible`.
- A `Logger` is owned by the caller (`delete` it). Its destructor tells the `UniqLogger`, which lowers the use count.
  At the last detach the writer is not deleted but made dormant (flushed, file closed, timer stopped, rotation
  state kept) and is reactivated by the next logger; `purgeUnusedWriters()` deletes the dormant ones with
  `deleteLater()`, because the writer and its timer belong to a pool thread.
- A `Logger` is used from many threads: message streaming (`<<` up to `EOM`) is buffered per calling thread, and a
  `Logger` has its own mutexes for the writer list and the buffers. A `LogWriter` has one `mutex` for its message
  list; `appendMessage` takes it from any thread, and the writer thread takes it when it writes.
- `flush()` of a logger or `flushAllWriters()` runs `writeToDevice()` in the calling thread, not in the pool
  thread. `UniqLogger::muxDeviceCounter` is held while writers are flushed or deactivated, so the lock order is
  `muxDeviceCounter` first, then the writer's `mutex`. Do not take them in the other order.
- `pauseLogging` sets a plain `bool` with no lock (as the code is today).
- The tests are built with `QTEST_MAIN`, `QTEST_GUILESS_MAIN` or `QTEST_APPLESS_MAIN`; whether the library needs a
  `QCoreApplication` in the consumer is not verified.
- A threading change plans a ThreadSanitizer run (skill `cpp-diagnostics`).

## 9. Generated and ignored files
- Git-ignored: `lib/config.pri`, `lib/depspath.pri`, `lib/src/ext`, `lib/debug`, `lib/release`, `Makefile*`, `build-*`,
  `*/build/*`, `*.pro.user`, `moc_*`, and everything in `lib/last_build` except its `.gitignore`. Check
  `git status` for `lib/xcframework_build` before you commit: it is not ignored.
- Nothing in the repository is generated and committed; the Doxygen output goes to `lib/last_build`.

## 10. Where new knowledge goes, here
The routing table is in `dev-workflow`. This repository's places for its rows: a rule for every change, `AGENTS.md`
"Conventions"; a dated result or baseline, a reference file of the skill (`baselines.md`, `pitfalls.md`); a change of
API, enum value or error code, the Doxygen comment and `README.md` with the version bump; why the code is the way it
is, a comment at the code or `README.md`; an open problem, an issue of the public repository or the pull request
description.
