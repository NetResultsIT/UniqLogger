##############################
#  UniqLogger Configuration  #
##############################

VERSION = 0.6.0

# --- Please check that the config file reflects your desired build options
!exists($$PWD/config.pri) {
    message("No config.pri found, building with default options: no NETWORK and no DB support")
} else: include ($$PWD/config.pri)

!exists($$PWD/depspath.pri) {
    error("No depspath.pri found, giving up")
} else: include ($$PWD/depspath.pri)


!exists($$FILECOMPRESSOR_ROOT/fileCompressor.pri) {
    error("Cannot find fileCompressor.pri: giving up")
} else: include ($$FILECOMPRESSOR_ROOT/fileCompressor.pri)

# ---- DO NOT CHANGE *ANYTHING* BELOW THIS LINE ---- #

QT -= gui

TARGET = UniqLogger

CONFIG += warn_on dll 
CONFIG += debug_and_release
CONFIG -= flat

DEFINES -= UNICODE

TEMPLATE = lib

#this should work with Qt5, on Qt4 we do it manually
#MYVER = $$split($$VERSION, .)
MYVER = 0
#message("$$QT_VERSION $$VERSION $$QMAKE_CXX $$QMAKESPEC")

message ("UniqLogger Version: $$VERSION")
message ("QT_VERSION $$QT_VERSION")

# --- Printing messages on building options
contains ( IDE, 'VS' ) {
    message("A Visual studio project file will be generated for UniqLogger")
}
else {
    message("Makefile(s) will be generated")
}

# ----- Library sources ------

HEADERS += \
    src/Logger.h \
    src/LogWriter.h \
    src/FileWriter.h \
    src/ConsoleWriter.h \
    src/UniqLogger.h \
    src/LogMessage.h \
    src/ext/tpool/nrthreadpool.h \
    src/DummyWriter.h \
    src/bufferofstrings.h \
    src/ConsoleColorScheme.h

SOURCES += \
    src/Logger.cpp \
    src/LogWriter.cpp \
    src/FileWriter.cpp \
    src/ConsoleWriter.cpp \
    src/UniqLogger.cpp \
    src/LogMessage.cpp \
    src/ext/tpool/nrthreadpool.cpp \
    src/DummyWriter.cpp \
    src/bufferofstrings.cpp \
    src/ConsoleColorScheme.cpp

# ----- Headers to export -----
INCLUDE_HEADERS += \
    $$PWD/src/UniqLogger.h \
    $$PWD/src/Logger.h \
    $$PWD/src/LogWriter.h \
    $$PWD/src/LogMessage.h \
    $$PWD/src/unqlog_common.h \
    $$PWD/src/bufferofstrings.h \
    $$PWD/src/ConsoleColorScheme.h


INCLUDEPATH += $$PWD/src $$PWD/src/ext/tpool

# --- UniqLogger Modules
contains ( DEFINES, ENABLE_UNQL_DBG ) {
    message ( "WARNING - The library will be built in DEBUG mode!!!" )
    #enable debug of tpool
    DEFINES += ENABLE_TPOOL_DBG
}

contains ( DEFINES, 'ENABLE_UNQL_NETLOG' ) {
    message("[*] Network Logging: ENABLED")
    QT += network
    HEADERS +=  src/RemoteWriter.h
    SOURCES += 	src/RemoteWriter.cpp
}
else {
    message("[ ] Network Logging: DISABLED")
}

contains ( DEFINES, 'ENABLE_UNQL_DBLOG' ) {
    message("[*] Db Logging:      ENABLED")
    QT += sql

    INCLUDEPATH += $$PWD/src/ext/dbh

    HEADERS +=  src/DbWriter.h \
                src/ext/dbh/nrdbhandler.h \
                src/UnqlDbHandler.h

    SOURCES +=  src/DbWriter.cpp \
                src/ext/dbh/nrdbhandler.cpp \
                src/UnqlDbHandler.cpp
}
else {
    message("[ ] Db Logging: DISABLED")
}
# --------

#Set our default compiler (Linux & Mac)
COMPILER = g++

win32-msvc2008 {
    message("Using VC++ 2008")
    COMPILER=VC2008
}

win32-msvc2010 {
    message("Using VC++ 2010")
    COMPILER=VC2010
}

win32-msvc2012 {
    message("Using VC++ 2012")
    COMPILER=VC2012
}

win32-msvc2013 {
    message("Using VC++ 2013")
    COMPILER=VC2013
}

win32-msvc2015 {
    message("Using VC++ 2015")
    COMPILER=VC2015
}

win32-msvc2017 {
    message("Using VC++ 2017")
    COMPILER=VC2017
}

message("COMPILER: $$COMPILER")

DSTDIR = $$PWD/last_build/
FINALDIR = $$join(COMPILER,,,_qt-$$QT_VERSION)
INCLUDE_DIR = $$DSTDIR/include/
DLLPATH = bin/


CONFIG(debug, debug|release) {
    message("Debug build")
    FINALDIR = $$join(FINALDIR,,"$$PWD/debug/","/")
}

CONFIG(release, debug|release) {
    message("Release build")
    FINALDIR = $$join(FINALDIR,,"$$PWD/release/","/")
}

message ("Library will be built in $$FINALDIR")
message ("Library will be copied also in $$DSTDIR")
message ("Actual library will be in $$FINALDIR$$DLLPATH")

win32 {
    message("Building UniqLogger library for Win32")
    #message("NOW USING COMPILER: $$COMPILER $$DSTDIR final: $$FINALDIR")
    CONFIG += flat

    contains(IDE,VS) {
      TEMPLATE = vclib
    }

    DEFINES += _CRT_SECURE_NO_DEPRECATE ULOG_LIB_EXPORTS

    LIBS += ws2_32.lib \
            Psapi.lib

    TARGET = $$join(TARGET,,$$DLLPATH)
    FINALDIR = $$replace(FINALDIR,"/","\\")
    TARGET = $$replace(TARGET,"/","\\")
    DSTDIR = $$replace(DSTDIR,"/","\\")


    WINEXT = dll lib exp

    #This is an hack to deal with non existing deploy dir on windows with Qt4, when we deprecate 4 we can delete it
    greaterThan(QT_MAJOR_VERSION, 6){
        QMAKE_POST_LINK="$$QMAKE_MKDIR_CMD \"$$FINALDIR\" $$escape_expand(\\n\\t)" # Does not work as expected with Visual Studio 2010
    } else {
        QMAKE_POST_LINK="$$QMAKE_CHK_DIR_EXISTS \"$$FINALDIR\" $$QMAKE_MKDIR \"$$FINALDIR\" $$escape_expand(\\n\\t)"
        QMAKE_POST_LINK="$$QMAKE_CHK_DIR_EXISTS \"$$DSTDIR\" $$QMAKE_MKDIR \"$$DSTDIR\" $$escape_expand(\\n\\t)"
        QMAKE_POST_LINK="$$QMAKE_CHK_DIR_EXISTS \"$$INCLUDE_DIR\" $$QMAKE_MKDIR \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"
    }

    #QMAKE_POST_LINK="$$WINCMD ..\\lib\\scripts\\mkDeployDir.bat $$FINALDIR $$escape_expand(\\n\\t)exit$$escape_expand(\\n\\t)"
    CONFIG(debug, debug|release) {
        TARGET = $$join(TARGET,,,d)
        DLL = $$join(TARGET,,debug\\,$$MYVER)
        WINEXT += pdb
    }
    CONFIG(release, debug|release) {
        message("******* Final release target is: $$TARGET")

        # needed to create a core dump
        QMAKE_CXXFLAGS_RELEASE += /Zi
        QMAKE_LFLAGS_RELEASE += /DEBUG
        QMAKE_LFLAGS_RELEASE += /OPT:REF
        QMAKE_LFLAGS_RELEASE += /OPT:ICF
        QMAKE_LIBDIR += $$WINDOWS_SDK

        DLL = $$join(TARGET,,release\\,$$MYVER)
    }

    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$FINALDIR\" $$escape_expand(\\n\\t)"
    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$DSTDIR\" $$escape_expand(\\n\\t)"
    for(inc, INCLUDE_HEADERS):QMAKE_POST_LINK+="$$QMAKE_COPY $${inc} \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"
}


#Common UNIX stuff
unix {
    CONFIG(debug, debug|release) {
        QMAKE_CFLAGS += -g
        QMAKE_CXXFLAGS += -g
        QMAKE_LFLAGS += -g
        OBJECTS_DIR = debug/build
        MOC_DIR = debug/build
    }
    CONFIG(release, debug|release) {
        OBJECTS_DIR = release/build
        MOC_DIR = release/build
    }
}



# Linux specific
unix:!macx:!ios:!android  {
    message("Building UniqLogger library for Linux / Unix")

    CONFIG(debug, debug|release) {
        DLL = $$join(TARGET,,lib,_d.so*)
        DLLPATH=$$join(DLLPATH,,debug/,)
        TARGET = $$join(TARGET,,$$DLLPATH,_d)
        DLL=$$join(DLL,,$$DLLPATH,)
    }
    CONFIG(release, debug|release) {
        DLL = $$join(TARGET,,lib,.so*)
        DLLPATH=$$join(DLLPATH,,release/,)
        TARGET = $$join(TARGET,,$$DLLPATH,)
        DLL=$$join(DLL,,$$DLLPATH,)
    }
    #message ("unix!macx!ios DLL $$DLL DLLPATH $$DLLPATH TARGET $$TARGET")

    android {
        message("Building UniqLogger for android")
    }
}



android {
    message("Building UniqLogger library for Android")
    equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
        message("Android Arch: armv7a")
        CONFIG(debug, debug|release) {
            LIBSUFFIX += _android_arm_debug
        }
        else {
            LIBSUFFIX += _android_arm
        }
    }
    equals(ANDROID_TARGET_ARCH, armeabi) {
        message("Android Arch: armeabi")
        error("Currently not supported")
    }
    equals(ANDROID_TARGET_ARCH, x86)  {
        message("Android Arch: x86")
        CONFIG(debug, debug|release) {
            LIBSUFFIX += _android_x86_debug
        }
        else {
            LIBSUFFIX += _android_x86
        }
    }

    CONFIG(debug, debug|release) {
        BLDTYPE=debug
    }
    CONFIG(release, debug|release) {
        BLDTYPE=release
    }

    TARGET = $$join(TARGET,,,$$LIBSUFFIX)
    DLL = $$join(TARGET,,lib,.so)
    DLLPATH=$$join(DLLPATH,,$$BLDTYPE/,)
    TARGET = $$join(TARGET,,$$DLLPATH,)
    DLL=$$join(DLL,,$$DLLPATH,)

    HEADERS += src/AndroidWriter.h
    SOURCES += src/AndroidWriter.cpp

    # Android doesn't support fopen64 & C.
    DEFINES += IOAPI_NO_64
}




# Mac OS X only
macx {
    message("Building UniqLogger library for MacOS")
    CONFIG(debug, debug|release) {
        TARGET = $$join(TARGET,,,_debug)
        BLDTYPE=debug
    }
    CONFIG(release, debug|release) {
        TARGET = $$join(TARGET,,,)
        BLDTYPE=release
    }

    DLL = $$join(TARGET,,lib,.*dylib)
    DLLPATH=$$join(DLLPATH,,$$BLDTYPE/,)
    TARGET = $$join(TARGET,,$$DLLPATH,)
    DLL=$$join(DLL,,$$DLLPATH,)

    #message ("macx DLL $$DLL DLLPATH $$DLLPATH TARGET $$TARGET")
}


# iOS specific
ios {
    message("Building UniqLogger library for iOS")
    IOSSUFFIX=_iOS

    lessThan(QT_VERSION, 5): error("You need at least Qt 5.9 to build vdk on iOS")
    lessThan(QT_MINOR_VERSION, 9): error("You need at least Qt 5.9 to build vdk on iOS")
    CONFIG += staticlib

    # armv7s is superset of armv7 and is ok since iOS6 on iphone 5 (we do not plan to support anything lower than iOS 10 and that's not going on iPhone 4)
    QMAKE_APPLE_DEVICE_ARCHS = armv7s arm64
    QMAKE_APPLE_SIMULATOR_ARCHS = x86_64 i386

    CONFIG -= bitcode

    CONFIG(debug, debug|release) {
        IOSSUFFIX = $$join(IOSSUFFIX,,,_debug)
        BLDTYPE=debug
    }
    CONFIG(release, debug|release) {
        BLDTYPE=release
    }

    TARGET = $$join(TARGET,,,$$IOSSUFFIX)
    DLL = $$join(TARGET,,lib,.a)
    DLLPATH=$$join(DLLPATH,,$$OUT_PWD/$$BLDTYPE/,)
    TARGET = $$join(TARGET,,$$DLLPATH,)
    DLL=$$join(DLL,,$$DLLPATH,)

    message ("ios DLL $$DLL DLLPATH $$DLLPATH TARGET $$TARGET suffix $$IOSSUFFIX")
}


# final UNIX common stuff
unix {
    QMAKE_POST_LINK += "mkdir -p $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK += "mkdir -p $$INCLUDE_DIR $$escape_expand(\\n\\t)"

    QMAKE_POST_LINK += "cp -aP $$DLL $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK += "cp -aP $$DLL $$DSTDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK += "cp -aP $$INCLUDE_HEADERS $$INCLUDE_DIR $$escape_expand(\\n\\t)"
}

message ("Library name: $$DLL")


QMAKE_CLEAN += -r
QMAKE_CLEAN += $$DLL $$DSTDIR/*
QMAKE_DISTCLEAN += $$QMAKE_CLEAN $$FINALDIR $$INCLUDE_DIR

message(" ==== End of UniqLogger QMake build process ==== ")

