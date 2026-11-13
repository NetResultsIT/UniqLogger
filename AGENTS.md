# UniqLogger workspace rules

UniqLogger is a Qt C++ logging library (LGPL-2, public on GitHub). It is built with qmake (`lib/uniqlogger.pro`),
as a shared library on Linux, macOS and Windows, a static library on iOS and a shared library on Android. Mobile
builds need Qt 6 (Android 6.8.4 or newer); the desktop code still has Qt 5 branches that are not verified on this
branch. The NetResults libraries VDK, KPE, ctilib and SSK link against it. Changes go to `master` of the public
repository through pull requests.

## 1. Other people and agents work here too
- Do not run git commands that move a checkout someone else uses (checkout, reset, clean, stash) and do not
  overwrite its build output (`lib/last_build`, `lib/debug`, `lib/release`, `lib/build`, `tests/build`) or its
  dependencies folder (`lib/src/ext`). Work in a separate git worktree unless told to use the current one.
- Keep each file's line endings. The sources in `example/` (`main.cpp`, `testlogger*.cpp`, `testlogger*.h`) are
  CRLF; everything else is LF. Check with `file` before editing a file you have not touched.
- Delete only what your own session created, by explicit path, never with a wildcard and never outside the project
  folder; ask before deleting anything else (branches, worktrees, build folders, dependency folders).
- Commit or push only when asked.
- This repository is public: no internal host names, URLs, credential paths or personal paths in any file.

## 2. Stop and ask first
You need the user's explicit consent before:
1. changing the public API: the headers installed for users (`UniqLogger.h`, `Logger.h`, `LogWriter.h`,
   `LogMessage.h`, `unqlog_common.h`, `bufferofstrings.h`, `ConsoleColorScheme.h`, `WriterConfig.h`, listed in
   `INCLUDE_HEADERS` of `lib/uniqlogger.pro` and in `build_ios_xcframework.sh`), every `ULOG_LIB_API` class, and the
   `VERSION` in `lib/uniqlogger.pro`. Other libraries cast their own enums to UniqLogger's (`LogMessagePriorityType`,
   `ErrorType`, the rotation, compression and protocol values of `WriterConfig`), so changing the value of an
   enumerator, reordering or inserting one, or resizing a public class breaks them silently. An API or ABI change
   needs a version bump (a separate commit "version bump to x.y.z");
2. changing threading: new threads, mutexes, lock order, the `NRThreadPool` use, the writer sharing and dormant
   state in `UniqLogger`, the slot names called through `invokeMethod` (`priv_startLogTimer`);
3. changing contracts: the file names written by `FileWriter` (the time rotation format macros in
   `unqlog_common.h` are matched by a regular expression in `FileWriter::removeLeftoversFromPreviousRun`), the log
   line format, the syslog message format, the database schema;
4. adding or removing files in the build files (`lib/uniqlogger.pro`, `lib/qt_yield_workaround.pri`,
   `lib/ut/unql-ut.pro`, `tests/tests.pro`), or changing the dependency list (`lib/deps.json`);
5. publishing or deleting anything outside the repository: tags, releases, packages, CI settings;
6. adding a third-party component or changing one's license. The library is LGPL-2 and public: the new component
   needs a compatible license, its license text must be kept with it, and the user confirms it fits the company
   policy ("it is the same license as X" is a suggestion, not the confirmation).
State what you want to change and why, and wait.

## 3. Lead and worker
Non trivial changes (a feature, a bug fix, a refactoring) follow skill `uniqlogger-dev-workflow`:
1. the lead model reads the code, designs the tests first, and writes the plan;
2. the lead stops at the gate of section 2 when the plan touches it;
3. a worker on a lighter model implements the plan and runs the tests;
4. the lead reviews the diff and the test output, and reports.

| Tool | Lead | Worker |
|---|---|---|
| Claude Code | the session model | `Agent` tool with a lighter model (`sonnet`, `haiku`) |
| Gemini CLI / Antigravity | the session model (Pro) | `invoke_subagent` with `Model: "flash"` |
| Codex and others | the session model | the tool's subagent or a second session on a smaller model |

A change of a few lines that the lead can verify alone does not need a worker.

## 4. Conventions
- The library does not print by itself: internal debug output uses `ULDBG` (compiled out unless
  `ENABLE_UNQL_DBG`), never a bare `qDebug()`. Failures of `create*Logger` and `get*Writer` are reported through the
  `int &ok` argument with a `UNQL::ErrorType` value.
- Writers are shared by destination inside one `UniqLogger` instance and live on pool threads; read the threading
  section of skill `uniqlogger-dev-workflow` before touching `UniqLogger.cpp`, `LogWriter.cpp` or `FileWriter.cpp`.
- Optional writers are compiled only with their switch (`ENABLE_UNQL_NETLOG`, `ENABLE_UNQL_DBLOG`,
  `ENABLE_UNQL_ANDROIDLOG`): guard their code with the same `#ifdef`, and test both ways.
- Do not edit `lib/config.pri`, `lib/depspath.pri` or `lib/src/ext`: they are local and git-ignored.
- Public methods carry a Doxygen comment (with `\version` for new API). When the public API or a documented behavior
  changes, update the comment, `README.md`, and `PROJECT_NUMBER` in `lib/Doxyfile` when the version changes.
- Source files start with the Kalliope S.r.l. copyright header; new files copy it.
- Python in `lib/create_pkg.py` and `scripts/flog2dblog.py` is Python 2 style (not verified to run on Python 3).

## 5. Skills
| Skill | Use it when |
|---|---|
| `uniqlogger-dev-workflow` | planning, implementing or reviewing any change; threading and API rules |
| `uniqlogger-build` | building the library for a platform, the XCFramework, or a package |
| `uniqlogger-run-tests` | running or writing the tests in `lib/ut` and `tests` |
| `qt-cpp` | writing or reviewing Qt C++: objects and threads, timers, signals, Qt 6.8 traps |

The skills are edited in `.agents/skills`; `.claude/skills` is a copy made by `scripts/sync_skills.py` (`--check` in
CI or a hook). The shared skills (`qt-cpp`) are copies from the `agent-skills` repository, pinned in
`.agents/shared-skills.lock`: they are not edited here.

## 6. Where new knowledge goes
What you learn while working (a missing build step, a trap, a wrong statement in these files) is written down in the
same change, in one place: the table is in skill `uniqlogger-dev-workflow`, "Where new knowledge goes". Correct a wrong
statement where it stands instead of adding a note. A worker reports what it learned; the lead decides where it
goes and reviews the edit like code.
