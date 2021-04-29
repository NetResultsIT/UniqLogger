QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

UNQLPATH = $$PWD/../../lib

INCLUDEPATH += $$UNQLPATH/src

SOURCES +=  tst_timeutilstickingchecks.cpp \
            $$UNQLPATH/src/TimeUtils.cpp
