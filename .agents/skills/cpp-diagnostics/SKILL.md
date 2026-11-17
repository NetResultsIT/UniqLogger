---
name: cpp-diagnostics
description: Dynamic checks of C and C++ code - sanitizers (ASan, UBSan, TSan), leak detection (LeakSanitizer, macOS leaks, Valgrind, Windows CRT debug heap) and CPU and heap profiling - which check a change needs, how to build, run and report it. Use when a change touches memory ownership, threads or performance, when a crash or leak is reported, and before claiming something is faster or leaks less.
---

# C and C++ diagnostics

A shared skill. The project's test skill holds what is specific to it: the build folder, the extra flags its code
needs, its suppression files, the scripts that run a sanitizer build, its baselines. When they say something
different, they add to this skill or make it stricter.

## Which check a change needs

| The change | Check | When |
|---|---|---|
| memory ownership, lifetimes, buffers, pointer arithmetic, parsing of external data | ASan + UBSan, and a leak check | planned in phase 1, run before the review |
| threads, locks, atomics, objects shared between threads, shutdown order | TSan | the same |
| a fix for a crash, a leak or a race | the check that shows it, before and after the fix | the reproduction comes first |
| a claim that something is faster, uses less memory or less CPU | a measurement before and after, same machine and build type | no claim without the numbers |
| a new third-party library or a version change | ASan + UBSan on the tests that use it | before the review |

Plan the checks in phase 1 with the tests, and say in the report which ran, where, and which did not.

## Sanitizers

Build in a separate folder: a sanitizer build must never replace the normal build output of the project.

| Sanitizer | clang and gcc | MSVC (not verified) | qmake (Qt 6) |
|---|---|---|---|
| AddressSanitizer | `-fsanitize=address` | `/fsanitize=address` (x86 and x64 only) | `CONFIG += sanitizer sanitize_address` |
| UndefinedBehaviorSanitizer | `-fsanitize=undefined` | not available | `CONFIG += sanitizer sanitize_undefined` |
| ThreadSanitizer | `-fsanitize=thread` | not available | `CONFIG += sanitizer sanitize_thread` |

- Compile and link with the same flag, add `-g -fno-omit-frame-pointer` (qmake's `sanitizer` feature adds the
  second) and build at `-O1` or higher for TSan.
- qmake needs both values: `sanitize_address` without `sanitizer` adds no flag.
- ASan and UBSan go together in one build. TSan goes in its own build: clang and gcc refuse `-fsanitize=address`
  with `-fsanitize=thread`.
- UBSan only prints and continues: add `-fno-sanitize-recover=undefined` so a finding fails the test, and run with
  `UBSAN_OPTIONS=print_stacktrace=1`.
- TSan sees only instrumented code. A race inside a library built without `-fsanitize=thread` (Qt, the
  dependencies) is missed, and synchronization done there can show up as false races. The project's test skill
  says whether instrumented builds of the dependencies exist; without them, report TSan results as partial.
- A clean run is not proof that the sanitizer worked: a toolchain can build with `-fsanitize=thread` and still
  report nothing. On a new compiler or machine, run once a small program with a known race (for ASan: a known leak)
  and check that it is reported; if not, use the other compiler, and say so in the report.
- TSan costs about 5 to 15 times the run time and 5 to 10 times the memory (clang documentation): raise the test
  timeouts, do not read a timeout under TSan as a failure of the code.
- Android (not verified): the Android documentation calls ASan unsupported since 2023 and recommends HWASan
  (64-bit Arm devices, with an OS requirement given there); the NDK and Qt's `sanitizer.prf` still contain ASan
  support.

## Leak detection

| Platform | Tool | How |
|---|---|---|
| Linux | LeakSanitizer, part of ASan | on by default in an ASan build; reports at exit and fails the run |
| macOS | `leaks` | `leaks -atExit -- <program> <args>` on a normal debug build; exit status 0 no leak, 1 leaks, more than 1 error |
| macOS | LeakSanitizer | not available with Apple clang on arm64 (`-fsanitize=leak` is refused, `detect_leaks=1` says "not supported"): use `leaks` |
| Linux | Valgrind memcheck | `valgrind --leak-check=full --error-exitcode=1 <program>`, on a normal debug build; slow, no macOS on arm64 |
| Windows | CRT debug heap (not verified) | `_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF \| _CRTDBG_LEAK_CHECK_DF)` at start, debug CRT only; MSVC's ASan has no leak detection |

- `leaks -atExit` sets `MallocStackLogging=lite` for the program, so the report shows where each leaked block was
  allocated. Do not run it inside an ASan build.
- A leak at exit of a singleton or of a library's global state is not always a bug: check whether the code frees it
  on purpose (a shutdown function, a destructor that runs too late). Report it with that analysis, do not suppress
  it on sight.
- A test program that leaks on every run hides new leaks. Note its baseline in the project's test skill and
  compare against it.

## Suppressions

- One file per tool, in the repository, used through the environment:
  `LSAN_OPTIONS=suppressions=<file>` (lines `leak:<pattern>`), `TSAN_OPTIONS=suppressions=<file>`
  (lines `race:<pattern>`), Valgrind `--suppressions=<file>`.
- Every entry has a comment above it: what it hides, why it is not a bug of this project (third-party code, an
  intended global), and the date. An entry for the project's own code needs the user's consent.
- Never widen a pattern to make a run green.

## Profiling

Measure on a release build with debug information (`-O2 -g`, qmake `CONFIG += force_debug_info`), on a quiet
machine, the same input before and after. Report the numbers, the machine, the build type and how many runs.

| Need | Tool |
|---|---|
| a micro benchmark inside the tests | Qt Test `QBENCHMARK { ... }`; options `-iterations n`, `-median n` |
| the benchmark's measurement | Qt Test `-tickcounter`, `-eventcounter`; `-callgrind` (needs Valgrind) and `-perf` on Linux |
| where the CPU time goes, Linux | `perf record --call-graph dwarf <program>`, then `perf report`; or Valgrind `--tool=callgrind` |
| where the CPU time goes, macOS | `xcrun xctrace record --template 'Time Profiler' --output <file>.trace --launch -- <program>`; a running process: `sample <pid> <seconds>` |
| where the CPU time goes, Windows (not verified) | `wpr -start CPU`, run, `wpr -stop <file>.etl`, open the file in Windows Performance Analyzer |
| heap use and allocations | Linux `heaptrack <program>`, then `heaptrack_print` or `heaptrack_gui`; Valgrind `--tool=massif`; macOS template `Allocations` of `xctrace` |

- `perf record` failing with a permission error ("Failure to open any events") means the machine's
  `kernel.perf_event_paranoid` setting forbids it: ask the machine's owner; do not change it yourself.
- A profile shows where the time goes in one run, not that a change is faster: for that, compare a benchmark
  before and after, several runs each, and give the spread.
- Do not optimize what the profile does not show as significant. Write the measurement in the commit message or
  the merge request.

## What to report

Which checks ran (tool, platform, build flags, test programs), the findings with their first frames in the
project's code, what each finding is (bug of this change, bug already there, third-party, false positive) and how
you know, the suppressions added, and which checks could not run and why.

Sources: clang.llvm.org (AddressSanitizer, LeakSanitizer, ThreadSanitizer, UndefinedBehaviorSanitizer),
learn.microsoft.com (MSVC AddressSanitizer, CRT debug heap, Windows Performance Recorder), valgrind.org,
developer.android.com (NDK sanitizers), doc.qt.io (Qt Test overview), the `leaks` and `xctrace` manual pages.
Checked by running them: the qmake values and the Apple clang behaviour (Qt 6.8.6, Apple clang 21, macOS arm64);
ASan leak reports, the LeakSanitizer suppression, UBSan with `-fno-sanitize-recover`, the ASan and TSan refusal,
Valgrind's exit code and heaptrack (gcc 16 and clang 22 on Linux x86_64). The Windows and Android lines come from
the vendors' documentation and were not run.
