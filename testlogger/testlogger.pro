
UNIQLOGGERPATH = $$PWD/../lib


##### DO NOT CHANGE BELOW THIS LINE ######

QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = bin/testlogger
INCLUDEPATH += . $$UNIQLOGGERPATH/src
CONFIG += debug_and_release

# Input
HEADERS += testlogger.h testlogger2.h \
    testlogger_zip.h
FORMS += testlogger.ui
SOURCES += main.cpp testlogger.cpp testlogger2.cpp \
    testlogger_zip.cpp


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

LIBS += -L$$UNIQLOGGERPATH/last_build

#
#CONFIG(release, debug|release) {
#    QMAKE_LIBDIR += $$UNIQLOGGERPATH/release/$$ULIBDIR
#}
#CONFIG(debug, debug|release) {
#    QMAKE_LIBDIR += $$UNIQLOGGERPATH/debug/$$ULIBDIR
#}

win32 {
    contains (IDE, VS) {
        TEMPLATE = vcapp
    }

    CONFIG += flat console
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK="copy ..\\lib\\debug\\$${COMPILER}_qt-$$QT_VERSION\\UniqLoggerd0.dll .\\debug\\bin\\ /y$$escape_expand(\n\t)"
        LIBS += UniqLoggerd0.lib
    } else {
        LIBS += UniqLogger0.lib
    }
}


unix:!macx,!ios {
    CONFIG(debug, debug|release) {
        LIBNAME = UniqLogger_d
    }
}

macx{
    CONFIG(debug, debug|release) {
        LIBNAME = UniqLogger_debug
    }
}


ios{
    message("Building testlogger for iOS")

    iphonesimulator {
        message("it will run on an iPhone Simulator")
        CONFIG(debug, debug|release) {
            LIBNAME = UniqLogger_iphonesimulator_debug
        }
    }

    iphoneos {

    }
}


android {
    message("Building testlogger for Android")
    equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
        message("Android Arch: armv7a")
        error("Currently not supported")
    }
    equals(ANDROID_TARGET_ARCH, armeabi) {
        message("Android Arch: armeabi")
        error("Currently not supported")
    }
    equals(ANDROID_TARGET_ARCH, x86)  {
        message("Android Arch: x86")
        CONFIG(debug, debug|release) {
            LIBNAME = UniqLogger_android_x86_debug
        }
    }
}


unix {
    LIBS += -l$$LIBNAME
}
