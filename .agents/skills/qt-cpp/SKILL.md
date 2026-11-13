---
name: qt-cpp
description: Checklist for writing and reviewing Qt 6 C++ in this library - object and thread ownership, connections across threads, timers, QSignalSpy in tests, types that cross the public API, JSON output, Qt 6.8 platform traps, and running without the application's QCoreApplication. Use before writing or reviewing code that creates QObjects or threads, connects signals, uses QTimer or sockets, or writes a Qt Test that waits for a signal.
---

# Qt 6 C++ checklist

This skill is shared by several Qt libraries. The file in a project is a copy from the `agent-skills` repository,
pinned in `.agents/shared-skills.lock`: do not edit it in the project (the skills check fails). To add or correct an item, change it in `agent-skills`, or describe it in the merge
request of the project you work on and a maintainer carries it over.

The project specific rules (naming, error codes, logging, which files are generated) are in `AGENTS.md` and the
other skills of this repository: when they say something different, they win.

Each item is here because it has already caused a bug or a flaky test in one of the libraries that share it.

## Objects and threads
- A `QObject` belongs to the thread it was created in, or the one it was moved to with `moveToThread()`. Its slots
  and timers run there. Before you add a member object, know which thread that is.
- Create an object that must live in a worker thread inside that thread (`run()` or a slot called there), or move it
  before it is used. Never call `moveToThread()` from a thread that does not own the object.
- A `QObject` with a parent cannot be moved, and parent and children must live in the same thread.
- `deleteLater()` needs an event loop in the object's thread. If that thread has finished, the object is never
  deleted: delete it directly from the owning code, after the thread has stopped.
- Do not `delete` an object while a queued call or a pending `finished()` for it is still in an event queue: stop
  the source first, or use `deleteLater()` from its own thread.
- `QThread` itself lives in the thread that created it, not in the thread it runs. Slots of a `QThread` subclass run
  in the creator's thread unless they are called from `run()`.
- Stop a thread in this order: ask it to stop (`quit()` or a flag), `wait()` with a timeout, check the result, then
  delete. Report the timeout as an error instead of calling `terminate()`.

## Timers
- A `QTimer` must be started and stopped from the thread it lives in. From another thread use
  `QMetaObject::invokeMethod(timer, ..., Qt::QueuedConnection)` or a signal.
- A timer needs the event loop of its thread: it never fires in a thread that blocks in a loop of its own.
- `QTimer::singleShot` with a lambda: pass a context object, so the call is dropped when the object is destroyed.
  A single shot that captures `this` without a context can fire after `this` is gone.

## Connections
- Typed connections only: `connect(sender, &Sender::sigX, receiver, &Receiver::onX)`. No `SIGNAL()` / `SLOT()`
  strings: they fail at run time, silently. Overloads with `qOverload<Args...>(&Class::method)`.
- A lambda slot takes a context object as third argument: it gives the thread the lambda runs in and disconnects it
  when the context is destroyed.
- Across threads the default (`Qt::AutoConnection`) is queued, and is decided when the signal is emitted, from the
  thread of the receiver. Write the connection type explicitly when the behaviour depends on it.
- `Qt::BlockingQueuedConnection` deadlocks when sender and receiver are in the same thread. Before using it, check
  the calling thread (`QThread::currentThread() == receiver->thread()`) and call directly in that case.
- `Qt::DirectConnection` across threads runs the slot in the emitting thread: the slot must be thread safe.
- Do not emit a signal that the application may connect to while holding a mutex: a directly connected slot that
  calls back into the library deadlocks.
- A type carried by a queued connection must be known to the meta type system: `Q_DECLARE_METATYPE` plus
  `qRegisterMetaType<T>()` for custom types and enums, done once before the first emission. Otherwise the call is
  dropped with only a warning on the console.
- Disconnect, or destroy the receiver, before the sender's data becomes invalid. `Qt::UniqueConnection` when the
  same connect can run twice (re-init paths).

## Tests with signals (Qt Test)
- `QSignalSpy::wait()` returns false when the signal has **already** been emitted before the call. Always write
  `QVERIFY(spy.count() > 0 || spy.wait(timeout))`, or `QTRY_VERIFY(spy.count() > 0)` / `QTRY_COMPARE`.
- Create the spy before the action that emits, and check `spy.isValid()`. Use the typed constructor
  `QSignalSpy(obj, &Class::sigX)`.
- No fixed `QTest::qSleep()` to wait for another thread: wait for the condition (`QTRY_*`) with a timeout. Fixed
  sleeps make the test slow on fast machines and flaky on emulated ones.
- A test that writes files uses a `QTemporaryDir`, not the working directory: tests run in the same folder, and
  some resources (log writers, databases) are shared by path within the process.
- A test must leave no thread, timer or socket alive: the next test of the same program inherits it.
- On macOS, tests that connect to `localhost` use `127.0.0.1` (see "Qt 6.8 platform traps").

## Types across the public API
- Strings in: `const QString &`. Strings out: `QString` by value. No raw `char *` owned by the caller.
- Integers with an explicit width (`qint32`, `quint16`, ...) or plain `int`; never `long` / `unsigned long` (their
  size changes between Windows and the other platforms). `qsizetype` stays internal: it is 32 bit on armv7.
- `char` and `int` convert into each other silently: two overloads that differ only by their order are ambiguous or,
  worse, both valid. Check the generated bindings (JNI passes `jchar`, `jint`, `jboolean`) when adding an overload.
- Getters are `const`. An error is reported through an `int &errorCode` (or `errCode`, as the project spells it)
  out parameter or a negative return value, never by an exception leaving the library.
- An enum used in a signal or a `Q_INVOKABLE` is declared with `Q_ENUM` / `Q_ENUM_NS` or registered as meta type.
- Keep third-party types (resiprocate, jrtplib, OpenSSL, sqlite, ...) out of the public headers.

## JSON produced for the application
- Build it with `QJsonObject` / `QJsonArray` / `QJsonDocument`, never by concatenating strings.
- Key names and value types are public API: renaming a key breaks the applications silently (they read nothing).
  A rename goes through the same gate as a method rename, with a line in the migration or porting notes.
- Parse input with `QJsonDocument::fromJson(data, &parseError)` and check both the error and `isObject()`;
  reject unknown keys and wrong types with an error, do not guess.
- Numbers in `QJsonValue` are doubles: integers above 2^53 do not survive. Carry ids of that size as strings.

## Running inside an application that is not Qt
- The library may be loaded by an application without a `QCoreApplication` (bindings for Objective-C, Java,
  Python, C#). The library then creates one: `argc` and `argv` must stay alive as long as the application object
  (static storage), and it must be created before any other `QObject` that needs it.
- Without the application's event loop running in the main thread, queued calls to objects that live in the main
  thread are never delivered. Objects that receive queued calls must live in a thread of the library that runs an
  event loop.
- Check `QCoreApplication::instance()` instead of assuming one or the other.

## Qt 6.8 platform traps
- **Apple Silicon, Xcode 26 SDK**: `qyieldcpu.h` of Qt up to 6.8 fails on `__yield()`. The projects force-include a
  workaround header (`qt_yield_workaround.pri` / `qt_arm_acle_workaround.h`): a new sub-project, test or example
  must include the same `.pri`, and so must any other project that compiles these headers.
- **macOS universal builds**: the libraries are built arm64 + x86_64. A test or tool linked against them must be
  built for an architecture the library contains (`lipo -archs`), and so must every dependency it loads.
- **macOS 26, refused TCP connects**: after a refused non-blocking connect Qt 6.8 can report the socket as
  connected (`connected()` with an empty peer address) and then `errorOccurred`; with a host name it does not try
  the next address, so `localhost` stops at `::1`. In code: do not trust `connected()` alone, check
  `peerAddress()` or wait for the first successful write/read. In tests: connect to `127.0.0.1`, and when a test
  expects "no connection", check for the error and not only for the absence of `connected()`.
- **iOS simulator**: Qt 6.8's iOS kit has no arm64 simulator slice (x86_64 only), so simulator builds run under
  Rosetta and need a universal simulator runtime.
- **Android**: include the Qt headers before third-party headers that define macros with common names (resiprocate
  defines `HashMap`, which breaks `qjnitypes.h`).

## Before you hand the change over
- Every new connection: typed, with a context for lambdas, and the connection type is right for the two threads.
- Every new timer, thread and `QObject`: who deletes it, in which thread, and what happens on shutdown and on a
  second `init()`.
- Every new test that waits: `count() > 0 || wait()` or `QTRY_*`, no fixed sleep, temporary files in a
  `QTemporaryDir`.
- Every change to a signal's signature, an enum value or a JSON key: it is a public API change.
