---
name: uniqlogger-dev-workflow
description: Procedure for any non trivial change to UniqLogger (feature, bug fix, refactoring) with a lead model and a worker, plus the rules of this library that every change must respect - the public API and enum values that other libraries cast, the writer threading model, line endings, licensing. Use before planning, implementing or reviewing a change in this repository.
---

# UniqLogger development workflow

Facts about the build and the tests are in `uniqlogger-build` and `uniqlogger-run-tests`. The gates that need the
user's consent are in `AGENTS.md` section 2.

## 1. Roles
The lead is the most capable model available. It reads the code, designs the tests, writes the plan, decides at
the gates, reviews the result and reports to the user. A worker is a subagent on a lighter, faster model: it
implements one plan, builds, runs the tests and reports. A change of a few lines that the lead can verify alone
does not need a worker.

## 2. Phase 1, plan (lead)
1. Check who else works here: `git worktree list`, `git status`, running builds. Work in your own worktree.
2. Read the code involved. The classes: `UniqLogger` (instances, writer registry, `NRThreadPool`), `Logger` (the
   user API, buffers messages per thread), `LogWriter` and its subclasses (`FileWriter`, `ConsoleWriter`,
   `DummyWriter`, and with switches `RemoteWriter`, `RSyslogWriter`, `DbWriter`, `AndroidWriter`), `WriterConfig`,
   `LogMessage`, `TimeUtils`, `SysLogMessageFactory`.
3. Design the tests first (see `uniqlogger-run-tests`, section 5): which program (`TestRegressionCoverage` for
   behavior of the public API, `TestFileWriterRotationScenarios` for rotation, `lib/ut` for one pure class), the cases
   (normal, boundary, wrong arguments and wrong state with the expected `ok` code, two loggers on one writer, a
   logger used from two threads, cleanup), and which `config.pri` switches the test needs.
4. Check the design rules (section 6) and the threading rules (section 7).
5. Write the plan: files and functions to change, tests, risks, which gate of `AGENTS.md` section 2 it touches.

## 3. Phase 2, gate (lead)
If the plan touches section 2 of `AGENTS.md`, stop and present: what changes, why, the alternatives, which
consumers are affected (other libraries cast the enums and link the
library), the version number you would
set, and how you tested. For a public API change also say whether the binary interface changes (size or layout of
an exported class, a virtual, a changed signature). Wait for the answer. For a third-party component also state its
name, version, license and where its license text will live; the user confirms the license fits the company policy.

## 4. Phase 3, implementation (worker)
The brief must contain:
- the checkout directory (a worktree) and the branch, and "do not commit";
- the files and the lines to change, and the production change in words;
- the tests to write first, and the case list from phase 1;
- the line endings: LF, except the CRLF files of `example/` (`main.cpp`, `testlogger*.cpp`, `testlogger*.h`);
- files not to touch: public enum values, `lib/config.pri`, `lib/depspath.pri`, `lib/src/ext`, the build files
  unless the plan says so;
- the exact build and test commands from `uniqlogger-build` and `uniqlogger-run-tests`, with the paths for this
  machine, and the baseline (what the tests give before the change, with the `Totals:` lines);
- what to report: files changed, deviations from the plan, the `Totals:` line of every program, what could not be
  built or run (a platform, a switch), and anything surprising.

## 5. Phase 4, review (lead)
Read the whole diff, rerun the tests yourself, then check:
- public headers, exported classes and enum values unchanged, or approved with a version bump;
- `ULOG_LIB_API` still on every exported class; no new public header without the user's consent (it must be added
  to `INCLUDE_HEADERS` and to `build_ios_xcframework.sh`);
- optional code guarded by its switch, and builds with and without the switches (at least the ones you can run);
- the threading rules below, and the `qt-cpp` checklist;
- error paths tested (the `ok` codes), cleanup in tests, no leftover files;
- line endings kept, no whitespace-only changes in CRLF files;
- Doxygen comments, `README.md` and (when the version changes) `lib/Doxyfile` updated;
- the new code has the copyright header like its neighbors;
- no internal host names, URLs or personal paths in anything added (the repository is public).
Report what you verified and what you could not (platforms, Windows, mobile).

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
- Other libraries cast their own enums to UniqLogger's: the levels of `UNQL::LogMessagePriorityType`, and the
  rotation policies, `compressionAlgo` and `netProtocol` values of `WriterConfig`. Never change the value of an
  existing enumerator and never insert one in the middle (enumerators without a value are numbered by position).
  Add new ones at the end, or with a new explicit value.
- Any API or ABI change needs the user's consent and a new `VERSION` in `lib/uniqlogger.pro`. The history bumps the
  version in its own commit named "version bump to x.y.z" (`git log --grep "version bump"`), after the change, and
  tags releases with the plain version. `lib/Doxyfile` `PROJECT_NUMBER` and `lib/ci/sonar-project.properties` hold
  older copies; do not treat them as the source of the version.
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

## 9. Generated and ignored files
- Git-ignored: `lib/config.pri`, `lib/depspath.pri`, `lib/src/ext`, `lib/debug`, `lib/release`, `Makefile*`, `build-*`,
  `*/build/*`, `*.pro.user`, `moc_*`, and everything in `lib/last_build` except its `.gitignore`. Check
  `git status` for `lib/xcframework_build` before you commit: it is not ignored.
- Nothing in the repository is generated and committed; the Doxygen output goes to `lib/last_build`.

## 10. Where new knowledge goes
Write down what you learn while working (a missing build step, a trap, a wrong statement in these files) in the
same change that produced it, in one place:

| What was learned | Where it is written |
|---|---|
| A rule that applies to every change here | `AGENTS.md` section 4, one line with a pointer |
| A procedure, or a step that was missing or wrong (build, tests, dependencies) | the `SKILL.md` of that skill, corrected in place |
| A dated result, a baseline, what one machine or VM did | a reference file of the skill (`baselines.md`, `pitfalls.md`) |
| A Qt trap, or anything else that belongs in a shared skill | the `agent-skills` repository, not the copy here (see below) |
| A change of API, JSON key, enum value or error code | the Doxygen comment and `README.md`, in the same change as the code, with the version bump |
| Why the code is the way it is (design, a race analysis) | a comment at the code, or `README.md` when users need it |
| An open problem, something not verified | an issue of the public repository, or the pull request description; not a skill |
| Something about one person's machine or preferences | not in the repository (the agent's own memory or local settings) |

- One place per fact: everything else points to it.
- Correct a wrong statement where it stands; do not append a note that contradicts it.
- A worker reports what it learned; the lead decides where it goes and reviews the edit like code.
- The shared skills (`qt-cpp`, `semver-abi`) and `sync_skills.py` are copies from the `agent-skills` repository, pinned in
  `.agents/shared-skills.lock`; the skills check fails if they are edited here. To change one, change it in
  `agent-skills`, or describe the change in your merge request here and a maintainer carries it over and updates
  the copies. Nothing in this repository needs a checkout of another project.

## 11. After the merge
Suggest to the user, and never do unasked: removing the worktree, the local branch and the scratch build folders you
created.
