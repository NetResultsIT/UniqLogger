
UNIQLOGGERPATH = ../lib


##### DO NOT CHANGE BELOW THIS LINE ######

QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = bin/testlogger
INCLUDEPATH += . $$UNIQLOGGERPATH/src
QMAKE_LIBDIR += $$UNIQLOGGERPATH/bin


# Input
HEADERS += testlogger.h testlogger2.h
FORMS += testlogger.ui
SOURCES += main.cpp testlogger.cpp testlogger2.cpp


LIBNAME = UniqLogger
#Set our default compiler (Linux & Mac)
COMPILER = g++

win32-msvc2008{
    message("Using VC++ 2008")
    COMPILER=VC2008
}

win32-msvc2010{
    message("Using VC++ 2010")
    COMPILER=VC2010
}

win32-msvc2012{
    message("Using VC++ 2012")
    COMPILER=VC2012
}

win32-msvc2013{
    message("Using VC++ 2013")
    COMPILER=VC2013
}

ULIBDIR = $$join(COMPILER,,,_qt-$$QT_VERSION)

CONFIG(release, debug|release) {
    QMAKE_LIBDIR += $$UNIQLOGGERPATH/release/$$ULIBDIR
}
CONFIG(debug, debug|release) {
    QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/$$ULIBDIR
}

win32 {
    contains (IDE, VS) {
        TEMPLATE = vcapp
    }

    CONFIG += flat
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK="copy ..\\lib\\debug\\$$COMPILER_qt-$$QT_VERSION\\UniqLoggerd.dll .\\debug\\bin\\ /y$$escape_expand(\n\t)"
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
