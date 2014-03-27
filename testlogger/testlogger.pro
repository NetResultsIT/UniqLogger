
UNIQLOGGERPATH = ../lib

##### DO NOT CHANGE BELOW THIS LINE ######

QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = bin/testlogger
INCLUDEPATH += . $$UNIQLOGGERPATH/src
QMAKE_LIBDIR += $$UNIQLOGGERPATH/bin


# Input
HEADERS += testlogger.h
FORMS += testlogger.ui
SOURCES += main.cpp testlogger.cpp


LIBNAME = UniqLogger

CONFIG(release, debug|release) {
    QMAKE_LIBDIR += $$UNIQLOGGERPATH/release/qt-$$QT_VERSION
}
CONFIG(debug, debug|release) {
    QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/qt-$$QT_VERSION
}

win32 {
    contains (IDE, VS) {
        TEMPLATE = vcapp
    }

    CONFIG += flat
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK="copy ..\\lib\\debug\\qt-$$QT_VERSION\\UniqLoggerd.dll .\\debug\\bin\\ /y$$escape_expand(\n\t)"
        LIBS += UniqLoggerd.lib
    } else {
        LIBS += UniqLogger.lib
    }
}

unix:!macx {
    CONFIG(debug, debug|release) {
        LIBNAME = UniqLogger_d
    }
}

macx{
    CONFIG(debug, debug|release) {
        LIBNAME = UniqLogger_debug
    }
}

unix {
    LIBS += -l$$LIBNAME
}
