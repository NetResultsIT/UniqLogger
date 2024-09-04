##############################
#  UniqLogger Configuration  #
##############################

VERSION = 1.3.0

# ---- DO NOT CHANGE *ANYTHING* BELOW THIS LINE ---- #

#if we populate version it seems that QT does not populate VER_MAJ/MIN/PAT automatically
VER_MAJ=$$section(VERSION, ., 0, 0)

!exists($$PWD/config.pri) {
    message("No config.pri found, building UniqLogger with default options: no NETWORK and no DB support")
} else: include ($$PWD/config.pri)

!exists($$PWD/depspath.pri) {
    message("No depspath.pri found, trying to see if UniqLogger dependencies are in default folder: $$PWD/src/ext")
} else: include ($$PWD/depspath.pri)


isEmpty(FILECOMPRESSOR_ROOT) {
    FILECOMPRESSOR_ROOT = $$PWD/src/ext/filecompressor/src
    message("Using default folder for filecompressor root: $$FILECOMPRESSOR_ROOT")
}

isEmpty(THREADPOOL_ROOT) {
    THREADPOOL_ROOT = $$PWD/src/ext/threadpool/src
    message("Using default folder for UniqLogger threadpool root: $$THREADPOOL_ROOT")
}

isEmpty(DBH_ROOT) {
    DBH_ROOT = $$PWD/src/ext/dbhandler/src
    message("Using default folder for UniqLogger dbhandler root: $$DBH_ROOT")
}

# Chek dependencies
!exists($$FILECOMPRESSOR_ROOT/nrFileCompressor.pri) {
    message("Cannot find nrFileCompressor.pri in folder $$FILECOMPRESSOR_ROOT")
    message("Please set in depspath.pri the var FILECOMPRESSOR_ROOT to the directory where you have a copy of NrFileCompressor")
    message("You can get a copy on GitHub at https://www.github.com/netresultsit/filecompressor")
    error("--- Missing nrFileCompressor.pri: giving up UniqLogger build ---")
} else: include ($$FILECOMPRESSOR_ROOT/nrFileCompressor.pri)


!exists($$THREADPOOL_ROOT/nrThreadPool.pri) {
    message("Cannot find nrThreadPool.pri...")
    message("Please set in depspath.pri the var THREADPOOL_ROOT to the directory where you have a copy of nrThreadPool")
    message("You can get a copy on GitHub at https://www.github.com/netresultsit/qt-threadpool")
    error("--- Missing nrThreadPool.pri: giving up UniqLogger build ---")
} else: include ($$THREADPOOL_ROOT/nrThreadPool.pri)


QT -= gui

TARGET = UniqLogger

CONFIG += warn_on dll
CONFIG += debug_and_release
CONFIG -= flat

versionAtLeast(QT_VERSION, 6.0.0): {
    CONFIG += c++17
}


DEFINES -= UNICODE

TEMPLATE = lib

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
    src/SysLogMessageFactory.h \
    src/TimeUtils.h \
    src/UniqLogger.h \
    src/LogMessage.h \
    src/DummyWriter.h \
    src/bufferofstrings.h \
    src/ConsoleColorScheme.h \
    src/WriterConfig.h

SOURCES += \
    src/Logger.cpp \
    src/LogWriter.cpp \
    src/FileWriter.cpp \
    src/ConsoleWriter.cpp \
    src/SysLogMessageFactory.cpp \
    src/TimeUtils.cpp \
    src/UniqLogger.cpp \
    src/LogMessage.cpp \
    src/DummyWriter.cpp \
    src/bufferofstrings.cpp \
    src/ConsoleColorScheme.cpp \
    src/WriterConfig.cpp

# ----- Headers to export -----
INCLUDE_HEADERS += \
    $$PWD/src/UniqLogger.h \
    $$PWD/src/Logger.h \
    $$PWD/src/LogWriter.h \
    $$PWD/src/LogMessage.h \
    $$PWD/src/unqlog_common.h \
    $$PWD/src/bufferofstrings.h \
    $$PWD/src/ConsoleColorScheme.h \
    $$PWD/src/WriterConfig.h


INCLUDEPATH += $$PWD/src

# --- UniqLogger Modules
contains ( DEFINES, ENABLE_UNQL_DBG ) {
    message ( "WARNING - The library will be built in DEBUG mode!!!" )
    #enable debug of tpool
    DEFINES += ENABLE_TPOOL_DBG
}

contains ( DEFINES, 'ENABLE_UNQL_NETLOG' ) {
    message("[*] Network Logging: ENABLED")
    QT += network
    HEADERS +=  src/RemoteWriter.h src/RSyslogWriter.h
    SOURCES +=  src/RemoteWriter.cpp src/RSyslogWriter.cpp
}
else {
    message("[ ] Network Logging: DISABLED")
}

contains ( DEFINES, 'ENABLE_UNQL_DBLOG' ) {
    message("[*] Db Logging:      ENABLED")
    QT += sql

!exists($$DBH_ROOT/nrDbHandler.pri) {
    message("Cannot find nrDbHandler.pri...")
    message("Please set in depspath.pri the var DBH_ROOT to the directory where you have a copy of nrDbHandler")
    message("You can get a copy on GitHub at https://www.github.com/netresultsit/qt-dbhandler")
    error("--- Missing nrDbHandler.pri: giving up UniqLogger build ---")
} else: include ($$DBH_ROOT/nrDbHandler.pri)


    HEADERS +=  src/DbWriter.h \
                src/UnqlDbHandler.h

    SOURCES +=  src/DbWriter.cpp \
                src/UnqlDbHandler.cpp
}
else {
    message("[ ] Db Logging: DISABLED")
}
# --------

#Set our default compiler (Linux & Mac)
COMPILER = $$QMAKE_CC

#message($$QMAKE_CC $$QMAKE_MSC_VER)
win32-msvc {
    message("Using VisualStudio compiler for UniqLogger, detecting version...")
    greaterThan(QMAKE_MSC_VER, 1929) {
        message("Using at least VC 2022")
        COMPILER=VC2022
    } else {
        greaterThan(QMAKE_MSC_VER, 1919) {
            message("Using VC 2019")
            COMPILER=VC2019
        } else {
            greaterThan(QMAKE_MSC_VER, 1909) {
                message("Using VC 2017")
                COMPILER=VC2017
            } else {
                greaterThan(QMAKE_MSC_VER, 1899) {
                    message("Using VC 2015")
                    COMPILER=VC2015
                } else {
                    message("WARNING: Using an old unsupported VC compiler")
                    COMPILER=VC_OBSOLETE
                }
            }
        }
    }
}

message("COMPILER: $$COMPILER")

DSTDIR = $$PWD/last_build/
FINALDIR = $$join(COMPILER,,$${VERSION}_$${QT_ARCH}_,_qt-$$QT_VERSION)
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
    contains(QT_ARCH, i386) {
        message("$$QT_ARCH 32-bit")
        message("Building UniqLogger library for Win322")
    } else {
        message("$$QT_ARCH 64-bit")
        message("Building UniqLogger library for Win64")
    }

    #message("NOW USING COMPILER: $$COMPILER $$DSTDIR final: $$FINALDIR")
    CONFIG += flat

    contains(IDE,VS) {
      message("Creating VC Project for Uniqlogger... use VS to compile")
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
        QMAKE_POST_LINK+="$$QMAKE_CHK_DIR_EXISTS \"$$DSTDIR\" $$QMAKE_MKDIR \"$$DSTDIR\" $$escape_expand(\\n\\t)"
        QMAKE_POST_LINK+="$$QMAKE_CHK_DIR_EXISTS \"$$INCLUDE_DIR\" $$QMAKE_MKDIR \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"
    }

    #QMAKE_POST_LINK="$$WINCMD ..\\lib\\scripts\\mkDeployDir.bat $$FINALDIR $$escape_expand(\\n\\t)exit$$escape_expand(\\n\\t)"
    CONFIG(debug, debug|release) {
        TARGET = $$join(TARGET,,,d)
        DLL = $$join(TARGET,,$$OUT_PWD\debug\\,$$VER_MAJ)
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

        DLL = $$join(TARGET,,$$OUT_PWD\release\\,$$VER_MAJ)
    }
    DLL = $$replace(DLL,"/","\\")

    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$FINALDIR\" $$escape_expand(\\n\\t)"
    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$DSTDIR\" $$escape_expand(\\n\\t)"
    #for(inc, INCLUDE_HEADERS):QMAKE_POST_LINK+="$$QMAKE_COPY \"$${inc}\" \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"

    INCLUDE_HEADERS = $$replace(INCLUDE_HEADERS,"/","\\")
    INCLUDE_DIR = $$replace(INCLUDE_DIR,"/","\\")
    #QMAKE_POST_LINK+="$$QMAKE_COPY \"$$INCLUDE_HEADERS\" \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"
    for(inc, INCLUDE_HEADERS):QMAKE_POST_LINK+="$$QMAKE_COPY \"$${inc}\" \"$$INCLUDE_DIR\" $$escape_expand(\\n\\t)"
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


# macOs specific
macx {
    greaterThan(QT_MAJOR_VERSION, 5) {
        message("Compiling UniqLogger on macOS Qt6 x86_64 only")
        QMAKE_APPLE_DEVICE_ARCHS="x86_64"
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
    if(equals(ANDROID_TARGET_ARCH, armeabi-v7a) | equals(ANDROID_TARGET_ARCH, arm64-v8a)){
        message("Android Arch: armv7a or arm64")
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
    equals(ANDROID_TARGET_ARCH, x86_64)  {
        message("Android Arch: x86_64")
        CONFIG(debug, debug|release) {
            LIBSUFFIX += _android_x86_64_debug
        }
        else {
            LIBSUFFIX += _android_x86_64
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

    DEFINES += ENABLE_UNQL_ANDROIDLOG

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
    message("MAJOR VERSION: $$VER_MAJ")
    CONFIG(release, debug|release) {
        TARGET = $$join(TARGET,,,)
        BLDTYPE=release
        QMAKE_POST_LINK += "install_name_tool -id @rpath/libUniqLogger.$${VER_MAJ}.dylib release/bin/libUniqLogger.dylib $$escape_expand(\\n\\t)"
    } else {
        QMAKE_POST_LINK += "install_name_tool -id @rpath/libUniqLogger_debug.$${VER_MAJ}.dylib debug/bin/libUniqLogger_debug.dylib $$escape_expand(\\n\\t)"
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

    # armv7s is superset of armv7 and is ok since iOS6 on iphone 5, 5c and ipad (2012) it introduces just a few
    # code optimization on those machines but limited and so was deprecated since Xcode 6.x
    # all newer iDevices (we do not plan to support anything lower than iOS 10 due to callkit) run arm64
    # so armv7s is not needed, armv7 is needed by app store, i386 is probably scrappable too if we run simulator on a new machine
    QMAKE_APPLE_DEVICE_ARCHS = armv7 arm64
    QMAKE_APPLE_SIMULATOR_ARCHS = x86_64 i386

    # Starting with Qt 5.12 minimum ios version is 11 and 32bit platforms are abandoned
    greaterThan(QT_MINOR_VERSION, 11){
        message("The Qt version selected ($$QT_VERSION) for this Uniqlogger build is too high for 32bit builds on iOS: building only 64bit")
        QMAKE_APPLE_DEVICE_ARCHS = arm64
        QMAKE_APPLE_SIMULATOR_ARCHS = x86_64
    }

    #Uncomment if you need to build w/o bitcode (Do this only if you know what you are doing!)
    #CONFIG -= bitcode
    contains(CONFIG, bitcode) : message("Building UniqLogger with BITCODE support")

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

    ios {
        !isEmpty(IOS_NOT_MOVE) {
            message("Not copying UniqLogger library in last_build and final dir because we need to perferm FAT lib creation")
            message("You will find the built lib in the creation dir")
        } else {
            message("UNQL normal deploy behaviour enabled on iOS")
            QMAKE_POST_LINK += "cp -aP $$DLL $$FINALDIR $$escape_expand(\\n\\t)"
            QMAKE_POST_LINK += "cp -aP $$DLL $$DSTDIR $$escape_expand(\\n\\t)"
            QMAKE_POST_LINK += $(RANLIB) $$DSTDIR/libUniqLogger_iOS.a $$escape_expand(\\n\\t)
        }
    } else {
        QMAKE_POST_LINK += "cp -aP $$DLL $$FINALDIR $$escape_expand(\\n\\t)"
        QMAKE_POST_LINK += "cp -aP $$DLL $$DSTDIR $$escape_expand(\\n\\t)"
    }
    QMAKE_POST_LINK += "cp -aP $$INCLUDE_HEADERS $$INCLUDE_DIR $$escape_expand(\\n\\t)"
}

message ("Library name: $$DLL")


QMAKE_CLEAN += -r
QMAKE_CLEAN += $$DLL $$DSTDIR/*
QMAKE_DISTCLEAN += $$QMAKE_CLEAN $$FINALDIR $$INCLUDE_DIR

message(" ==== End of UniqLogger QMake build process ==== ")

