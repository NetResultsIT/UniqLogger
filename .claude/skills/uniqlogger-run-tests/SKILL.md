---
name: uniqlogger-run-tests
description: How to build and run the UniqLogger tests (the small unit tests in lib/ut and the functional Qt Test programs in tests), which ones depend on config.pri switches or on time, how to read the output, and how to add a test. Use when you must run the tests, check a baseline before a change, or write a test for a fix.
---

# Running the UniqLogger tests

All tests are Qt Test programs built with qmake. They link the built library, so build the library first with skill
`uniqlogger-build` (same Qt, same `lib/config.pri`, same debug or release) and keep `lib/last_build` current.
`QT_ROOT` and `UNQL_SRC` are defined in the Local environment section of `uniqlogger-build`.

## 1. Two groups
- `lib/ut` (project `lib/ut/unql-ut.pro`, a `subdirs` project): unit tests of one class each
  (`ut_wconfig` for `WriterConfig`, `ut_syslog` for the syslog message factory). They link `lib/last_build`.
- `tests` (project `tests/tests.pro`): functional tests that create loggers and writers, write files in the
  current folder and check them. One folder per program: every `tests/Test*` folder has its own `.pro` and one
  `tst_*.cpp`. `tests/tests_common.pri` holds the library path and the macOS rpath.
  - `TestRegressionCoverage` is the broad regression test of the public behavior (instances, shared and dormant
    writers, tags, rotation state, network and database writers). Add regression tests for fixes there.
  - `TestFileWriterRotationScenarios` subclasses `FileWriter` and drives the rotation with injected dates, so it
    does not wait for real time.

## 2. Build and run
From a build folder outside the source (`$UNQL_SRC/tests/build/<kit>` is git-ignored):
```
$QT_ROOT/macos/bin/qmake -o Makefile ../../tests.pro -spec macx-clang CONFIG+=debug
make -j8
```
Run each program from its own folder in the build tree (the tests write their files in the current folder):
`cd TestLogPause && ./TestLogPause`. One test function: `./TestRegressionCoverage <function name>`.
`CONFIG += testcase` is set, so `make check` should run them all (not verified).
- Use the same build type as the library: debug links `libUniqLogger_debug` on macOS and `libUniqLogger_d` on
  Linux; release links `libUniqLogger`. `tests_common.pri` adds the library folder and the rpath only on macOS and
  Windows: on Linux you may need `LD_LIBRARY_PATH` and `-L` (not verified).
- On Windows the tests compile the internal classes they need into the test, because the DLL does not export them.
- `TestRegressionCoverage` includes `lib/config.pri`: its network and database cases exist only when the same
  switches (`ENABLE_UNQL_NETLOG`, `ENABLE_UNQL_DBLOG`) were used for the library. A mismatch fails at link time or
  silently drops the cases: compare the number of test functions run with the expected one.
- `lib/ut` is built the same way from `lib/ut/unql-ut.pro`. It links `-lUniqLogger` with the platform suffix.

## 3. Reading results
- Every program ends with `Totals: N passed, M failed, K skipped`. Report these lines, not "it passed".
- A program that crashes or hangs prints no `Totals:` line: that is a failure, and a hang usually means a writer
  thread did not stop.
- Compare with a baseline: run the same programs on the unchanged branch before you change code, and report both.
- Leftover files from a failed run (log files, `.zip`, `.gz` in the test folder) can fail the next run: delete the
  ones the run created, by explicit path.

## 4. Timing
- `TestMessagesCompression` sleeps one second between ten messages and `TestDummyDelete` sleeps a second after
  deleting a logger; `TestRegressionCoverage` waits with `QTest::qWait(100)` for writers to flush and
  `waitForReadyRead(300)` on a UDP socket in the syslog cases. A slow or loaded machine can fail them: rerun alone
  before calling a failure a regression.
- `TestTimeUtilsTicking` compares against the current time; run it twice around a minute, hour or day boundary if
  it fails once.
- Writers flush on a timer (`writerFlushSecs`, default 5): in a test, call `flush()` or
  `UniqLogger::flushAllWriters()` before reading a file, and set `writerFlushSecs = 1` when the test waits.
- Never fix a flaky test by lengthening a sleep without saying so in the report.

## 5. Writing a test
- Test first: write it, run it on the unchanged code and see it fail for the right reason.
- Use `uniqueName()` style instance names so tests do not share a `UniqLogger` instance (instances are process wide
  and named; `UniqLogger::instance` ignores a different thread count for an existing name).
- Cover: normal use, boundary values of `WriterConfig`, wrong arguments and wrong state with the expected `ok`
  code (`UNQL::ErrorType`), two loggers on the same file (shared writer), logger deleted then recreated (dormant
  writer), a logger used from two threads, and cleanup of files and loggers (`delete logger`, then
  `purgeUnusedWriters()`).
- Public enum values are part of the contract: a test may assert the numeric values of
  `LogMessagePriorityType` and the `WriterConfig` enums.
- New test program: it needs a `.pro` and an entry in `tests/tests.pro` (build file change: ask first).
