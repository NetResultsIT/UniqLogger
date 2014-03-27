QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += debug

UNIQLOGGERPATH = ../lib

TEMPLATE = app
TARGET = bin/testlogger
INCLUDEPATH += . $$UNIQLOGGERPATH/src
QMAKE_LIBDIR += $$UNIQLOGGERPATH/bin
LIBS += -lUniqLogger

# Input
HEADERS += testlogger.h
FORMS += testlogger.ui
SOURCES += main.cpp testlogger.cpp

win32 {

contains (IDE, VS) {
    TEMPLATE = vcapp
}
CONFIG += flat
LIBS += UniqLogger.lib
CONFIG(release, debug|release) {
QMAKE_LIBDIR += $$UNIQLOGGERPATH/release/bin
}
CONFIG(debug, debug|release) {
QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/bin
}
}
