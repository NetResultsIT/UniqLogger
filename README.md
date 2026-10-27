# UniqLogger
A Qt-based x-platform logging library with multiple backends and features

The UniQLogger library is a (thread-safe) Qt-based library that allows the logging and monitoring
functionalities for Qt-based projects. You can create various loggers (with different
module names and logging times) and associate them to different logwriters so that the
same messages can be dispatched to various devices (Console, Network, RSysLog, Database, File or Android).

UniqLogger compiles and works on Linux, MacOs, Windows, iOS and Android, other Qt-supported
platforms might work but are untested.

Network writers are clients over TCP connections (with optional TLS encryption)
Console writers can have different colors (currently not on windows)
File writers have the ability to use log-rotation (time and/or size based) and file compression (both zip and gzip)

Loggers have different levels of logging and you can be set so that a minimum level has to be achieved
before the actual message gets dispatched to the writers.

It's also possible to define the timestamp format in order to show milliseconds.

Monitoring is another cool feature of the UniQLogger library, it allows to define a map
of variables (identified by a keyword that must be unique per logger) that can be \i turned on
and off by the UniQLogger monitorVar() method.

There is also a Dbviewer tool to search through db-logged messages and a python script to import file-based
logs into a db.

Each `UniqLogger` instance is identified by a name. The default instance name is `UNQL`.
You can call `setDefaultPrintTag(true)` on a `UniqLogger` instance to enable tag printing for loggers
created afterwards, or call `printTag(true)` directly on a `Logger` to control that logger only.

### Writer Lifecycle and Shared Writers

Log writers (such as `FileWriter`, `ConsoleWriter`, `RemoteWriter`) are managed centrally by each `UniqLogger` instance and shared across loggers targeting the same destination.

- **Dormant State**: When all loggers referencing a writer are destroyed or detached, the writer transitions to a dormant state. The writer synchronously flushes and closes its underlying file or device handles and pauses background timers, while preserving its state (such as file rotation counters and previously rotated filenames). If a new logger is subsequently created for the same destination, the dormant writer is transparently reactivated.
- **Resource Reclamation (`purgeUnusedWriters`)**: If an application creates loggers with dynamic, non-reusable file paths and wants to reclaim memory from dormant writers, it can call `UniqLogger::purgeUnusedWriters()`. Active writers with attached loggers are untouched.
- **Flushing Writers (`flushAllWriters`)**: Calling `UniqLogger::flushAllWriters()` synchronously requests all registered writers to flush any queued messages.

Refer to lib/00-Compile file for compilation instructions

In the example folder there is a sample app project that can be compiled to show the library features
In the tests folder there are some unit and functional tests 

License is LGPL-2, if you need a commercial license, feel free to contact us.

Contributors are welcome :)
