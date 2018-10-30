# UniqLogger
A Qt-based x-platform logging library with multiple backends and features

The UniQLogger library is a (thread-safe) Qt-based library that allows the logging and monitoring
functionalities for Qt-based projects. You can create various loggers (with different
module names and logging times) and associate them to different logwriters so that the
same messages can be dispatched to various devices (Console, Network or File).

UniqLogger compiles and works on Linux, MacOs, Windows, iOS and Android, other Qt-supported
platforms might work but are untested.

Network writers are clients over TCP connections
Console writers can have different colors (currently not on windows)
File writers have the ability to use log-rotation and file compression (both zip and gzip)

Loggers have different levels of logging and you can be set so that a minimum level has to be achieved
before the actual message gets dispatched to the writers.

It's also possible to define the timestamp format in order to show milliseconds.

Monitoring is another cool feature of the UniQLogger library, it allows to define a map
of variables (identified by a keyword that must be unique per logger) that can be \i turned on
and off by the UniQLogger monitorVar() method.

There is also a Dbviewer tool to search through db-logged messages and a python script to import file-based
logs into a db.

Refer to 00-Compile file for compilation instructions

License is LGPL-2, if you need a commercial license, feel free to contact us.

Contributors are welcome :)
