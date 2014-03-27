QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += debug

UNIQLOGGERPATH = ../lib

TEMPLATE = app
TARGET = bin/testlogger
INCLUDEPATH += . $$UNIQLOGGERPATH/src
QMAKE_LIBDIR += $$UNIQLOGGERPATH/bin


# Input
HEADERS += testlogger.h
FORMS += testlogger.ui
SOURCES += main.cpp testlogger.cpp

##### DO NOT CHANGE BELOW THIS LINE ######

LIBNAME = UniqLogger

win32 {

    contains (IDE, VS) {
        TEMPLATE = vcapp
    }

    CONFIG += flat
    CONFIG(release, debug|release) {
        QMAKE_LIBDIR += $$UNIQLOGGERPATH/release/bin
    }
    CONFIG(debug, debug|release) {
        QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/bin
        QMAKE_POST_LINK="copy ..\\lib\\debug\\bin\\UniqLoggerd.dll .\\debug\\bin\\ /y$$escape_expand(\n\t)"
        LIBNAME = UniqLoggerd
    }

    LIBS += UniqLoggerd.lib
}

unix:!macx {
    CONFIG(debug, debug|release) {
        QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/bin
        LIBNAME = UniqLogger_d
    }
}

macx{
    CONFIG(debug, debug|release) {
        LIBNAME = UniqLogger_debug
    }
}

unix {
    CONFIG(debug, debug|release) {
        QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/bin
        LIBS += -l$$LIBNAME
    }
}
