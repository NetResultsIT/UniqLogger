# UniqLogger workspace rules

UniqLogger is a Qt C++ logging library (LGPL-2.1, public on GitHub). It is built with qmake (`lib/uniqlogger.pro`),
as a shared library on Linux, macOS and Windows, a static library on iOS and a shared library on Android. Mobile
builds need Qt 6 (Android 6.8.4 or newer); the desktop code still has Qt 5 branches that are not verified on this
branch. Other libraries link against it. Changes go to `master` of the public repository through pull requests.

## Project facts
The core rules below refer to these facts.
- Language level: C++17 with Qt 6 (`CONFIG += c++17` in `lib/uniqlogger.pro`); Android needs Qt 6.8.4 or newer
  (checked there). Raising either is `gate.language-level`.
- Public API (`gate.api`): the installed headers (`UniqLogger.h`, `Logger.h`, `LogWriter.h`, `LogMessage.h`,
  `unqlog_common.h`, `bufferofstrings.h`, `ConsoleColorScheme.h`, `WriterConfig.h`: `INCLUDE_HEADERS` of
  `lib/uniqlogger.pro` and `build_ios_xcframework.sh`) and every `ULOG_LIB_API` class. Other libraries cast their own
  enums to UniqLogger's (`LogMessagePriorityType`, `ErrorType`, the rotation, compression and protocol values of
  `WriterConfig`): changing, reordering or inserting an enumerator, or resizing a public class, breaks them silently.
  Other contracts: the file names written by `FileWriter` (the rotation format macros of `unqlog_common.h` are
  matched by a regular expression in `FileWriter::removeLeftoversFromPreviousRun`), the log line format, the syslog
  message format, the database schema.
- Versioning (skill `semver-abi`): `VERSION` in `lib/uniqlogger.pro`; 1.x is released, so the rule applies to every
  change. A changed or removed public API is a major, made only when the user explicitly acknowledges it (the
  compatible way, a new API with the old one forwarding, is proposed first). The bump is its own commit "version bump
  to x.y.z".
- Concurrency (`gate.concurrency`): new threads or mutexes, the lock order, the `NRThreadPool` use, the writer sharing
  and dormant state in `UniqLogger`, the slot names called through `invokeMethod` (`priv_startLogTimer`); the model is
  in skill `uniqlogger-dev-workflow`, "Threading model".
- Security-sensitive code (`gate.security`): no authentication or cryptography. What leaves the process or stays on
  disk: the network writers (`RemoteWriter`, `RSyslogWriter`), `DbWriter`, and the removal of old log files.
- Build files (`gate.build`): `lib/uniqlogger.pro`, `lib/qt_yield_workaround.pri`, `lib/ut/unql-ut.pro`,
  `tests/tests.pro`; dependency list `lib/deps.json`.
- Build output and dependency folders (`shared-checkout`): `lib/last_build`, `lib/debug`, `lib/release`, `lib/build`,
  `tests/build`; dependencies in `lib/src/ext`. Worktrees can run side by side.
- Line endings: LF, except these files of `example/`, which are CRLF: `main.cpp`, `testlogger.cpp`, `testlogger.h`,
  `testlogger.ui`, `testlogger2.cpp`, `testlogger2.h`.
- Outside the repository (`gate.outside`): tags, releases, packages, the GitHub settings and workflows.
- Third-party (`gate.third-party`, skill `third-party`): the library's license is `LICENSE` (LGPL-2.1); a new component
  needs a license compatible with it and its license text kept with it. The dependencies of `lib/deps.json` are
  fetched with their own files (not verified). License report: none. Approved licenses: no list.
- Architecture document: missing (the threading model is in skill `uniqlogger-dev-workflow`); design notes: code
  comments, or `README.md` when users need them.
- Naming rules: skill `qt-naming`, integers relaxed; check: none.
- Tests: `lib/ut` and `tests` (skill `uniqlogger-run-tests`); coverage: not measured.
- Documents updated with the code: the Doxygen comments, `README.md`, and `PROJECT_NUMBER` in `lib/Doxyfile` when the
  version changes.

## Conventions
- This repository is public: no internal host names, URLs, credential paths or personal paths in any file.
- The library does not print by itself: internal debug output uses `ULDBG` (compiled out unless
  `ENABLE_UNQL_DBG`), never a bare `qDebug()`. Failures of `create*Logger` and `get*Writer` are reported through the
  `int &ok` argument with a `UNQL::ErrorType` value.
- Optional writers are compiled only with their switch (`ENABLE_UNQL_NETLOG`, `ENABLE_UNQL_DBLOG`,
  `ENABLE_UNQL_ANDROIDLOG`): guard their code with the same `#ifdef`, and test both ways.
- Do not edit `lib/config.pri`, `lib/depspath.pri` or `lib/src/ext`: they are local and git-ignored.
- Public methods carry a Doxygen comment (with `\version` for new API).
- Source files start with the Kalliope S.r.l. copyright header; new files copy it.
- Python in `lib/create_pkg.py` and `scripts/flog2dblog.py` is Python 2 style (not verified to run on Python 3).

## Skills
| Skill | Use it when |
|---|---|
| `uniqlogger-dev-workflow` | planning, implementing or reviewing any change (extends `dev-workflow`): threading, API |
| `dev-workflow` | the shared procedure: plan with tests first, plan challenge, gate, worker brief, review, report |
| `third-party` | existing components before new code; adding or updating a dependency; licenses |
| `agentify` | filling in or improving the project facts, the conventions and these files with the maintainer |
| `uniqlogger-build` | building the library for a platform, the XCFramework, or a package |
| `uniqlogger-run-tests` | running or writing the tests in `lib/ut` and `tests` |
| `qt-cpp` | writing or reviewing Qt C++: objects and threads, timers, signals, Qt 6.8 traps |
| `qt-naming` | the naming rules: types, members, methods, signals, parameters, integer types |
| `cpp-diagnostics` | sanitizers, leak checks, profiling: which check a change needs and how to report it |
| `semver-abi` | a change touches public API, an enum, a JSON format or an error code; adding a feature; a release |

The skills are edited in `.agents/skills`; `.claude/skills` is a copy made by `scripts/sync_skills.py`; the pre-commit
hook runs its `--check` (enable it once per clone: `git config core.hooksPath .githooks`). The shared skills
(`dev-workflow`, `third-party`, `agentify`, `qt-cpp`, `qt-naming`, `cpp-diagnostics`, `semver-abi`) and the core rules
block below are copies from the `agent-skills` repository, pinned in `.agents/shared-skills.lock`: they are not edited
here.

<!-- agent-skills:core begin: written by agent-skills, do not edit here (repair: sync_skills.py --fix) -->
## Core rules
The same in every repository that uses `agent-skills`. Each rule has an id. The project's own sections and skills
may add to a rule or make it stricter, never relax it: a rule is relaxed only by an entry in "Overrides of the core
rules" below, written by the maintainers, one line each:
`` - `<id>`: <what changes>. Reason: <why>. Agreed: <who>, <YYYY-MM-DD>. ``
An agent may draft such a line; the reason and the name of who agreed come from the user, never from the agent.

### Other people and agents work here too
- `shared-checkout`: do not run git commands that move a checkout someone else uses (`checkout`, `reset`, `clean`,
  `stash`) and do not overwrite the build output or dependency folders named in "Project facts". Work in a separate
  git worktree unless told to use the current one, or "Project facts" says worktrees cannot run side by side.
- `line-endings`: keep each file's line endings; new files are LF.
- `delete`: delete only what your own session created, by explicit path, never with a wildcard and never outside the
  project folder; ask before deleting anything else.
- `commit`: commit or push only when asked; never bypass a hook (`--no-verify`).
- `secrets`: never write a secret, a token or a password into a tracked file, a log, a report or a commit message.
- `language`: agent files (`AGENTS.md`, skills) are written in English; existing documents in other languages are
  pointed to as they are.

### How to work
- `truth`: when the user's request, design or plan has a flaw, say so, what it costs and what you would do instead;
  then do what the user decides. Do not agree to please.
- `no-guess`: say when you do not know or could not check. In the final report of a task, mark each statement as
  verified (say how), not verified, or assumed. Guess only when asked, and label the guess.
- `quality`: do not shorten the work to answer sooner (read the code involved, run the tests, check before you
  claim) unless the user asks for a quick answer. Done means: what was asked works, is tested and is reported.
  Do not widen the scope: propose the rest.
- `tests-first`: a feature or a change of behaviour comes with a test that fails without it; a bug fix starts with a
  test that reproduces the bug. Where coverage is measured, changed lines are covered and the total does not drop.
  Going without a test needs the user's explicit request, written in the plan and in the merge request description.
- `reuse`: before writing more than a few dozen lines that are not specific to this project, look in the project,
  in its dependencies and for an existing third-party component, and propose what you find (skill `third-party`).
- `design-docs`: the architecture document and the naming rules are named in "Project facts". A new module,
  service, process, protocol or data format gets a short design note before the code. When a document is missing,
  say so and propose to write it with the maintainer (skill `agentify`).
- `web-search`: web searches and page reading go to a subagent on the lightest model available (table below); it
  returns URLs and quoted passages. The lead judges them and reads itself the pages a decision rests on.

### Stop and ask first
You need the user's explicit consent before:
1. `gate.api`: changing a public API or another contract listed in "Project facts" (formats, protocols, database
   schema, base classes); name the consumers listed there that it affects; a breaking change follows the
   versioning named there, compatible way proposed first;
2. `gate.concurrency`: changing threads, processes, locks or their order, event loops or background workers;
3. `gate.security`: changing authentication, authorization, cryptography, the handling of secrets, or how personal
   data is stored, logged, exported or retained;
4. `gate.language-level`: raising the language level or the oldest compiler, runtime, framework or platform
   supported (the limits are in "Project facts");
5. `gate.build`: adding or removing files in the build files, or changing the dependency lists;
6. `gate.outside`: changing anything outside the repository: publishing, uploading or deleting packages, tags,
   deployments, CI settings, shared databases;
7. `gate.third-party`: adding a third-party component or a version whose license changed; do not commit it before
   the user acknowledges the license (skill `third-party`);
8. `gate.rules`: changing this block, the overrides, the hooks or the CI checks.
A request to make such a change is not yet the consent: first say which item it touches, what it costs and which
"Project facts" it conflicts with, then wait for the user to confirm. State what you want to change, why, which
files and which tests.

### Lead and worker
- `workflow`: a non trivial change (feature, bug fix, refactoring) follows skill `dev-workflow` and the project's
  workflow skill: the lead reads the code, designs the tests first and writes the plan; stops at the gate when the
  plan touches it; a worker on a lighter model implements the plan and runs the tests; the lead reviews the diff,
  reruns the tests and reports. A change of a few lines that the lead can verify alone needs no worker.
- `plan-challenge`: for a complex task (see `dev-workflow`) the lead proposes to have the plan challenged by a
  second high-capacity model, which gets the plan and the code, not the lead's reasoning.

| Tool | Worker, web search | Challenger |
|---|---|---|
| Claude Code | `Agent` tool with `model`: `sonnet` or `haiku` (web search: `haiku`) | ask the user to start another tool; else `Agent` tool, `opus` |
| Codex | a subagent whose model is set in `.codex/agents/<name>.toml`, when the project has one | ask the user to start another tool |
| Antigravity | a subagent in `.agents/agents/<name>.md` with `model: flash`, when the project has one | ask the user to start another tool |

When no lighter subagent is available, the session does the work itself and says so in the report.

### Where new knowledge goes
- `knowledge`: what you learn while working (a missing step, a trap, a wrong statement in these files) is written
  down in the same change, in one place: the table is in skill `dev-workflow`. Correct a wrong statement where it
  stands. A worker reports what it learned; the lead decides where it goes. Shared skills and this block are
  changed in the `agent-skills` repository, not here.
- `missing-skill`: when a skill named in these rules or in another skill is not in `.agents/skills`, say so before
  you go on, do not invent its procedure, and propose that a maintainer update the shared skills from
  `agent-skills` with `update_project.py` (the version installed here is in `.agents/shared-skills.lock`).
<!-- agent-skills:core end -->

## Overrides of the core rules
None.
