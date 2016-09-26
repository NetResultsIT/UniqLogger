##############################
#  UniqLogger Configuration  #
##############################

VERSION = 0.3.3

# --- Please check that the config file reflects your desired build options
!exists(config.pri) {
    message("No config.pri found, building with default options: no net and no DB support")
} else: include (config.pri)

!exists(depspath.pri) {
    error("No depspath.pri found, giving up")
} else: include (depspath.pri)

!exists($$PWD/filecompressor/trunk/fileCompressor.pri) {
    error("Cannot find fileCompressor.pri: giving up")
} else: include ($$PWD/filecompressor/trunk/fileCompressor.pri)

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
    message("A Visual studio project file will be generated")
}
else {
    message("Makefile(s) will be generated")
}

contains ( DEFINES, ULOGDBG ) {
    message ( "WARNING - The library will be built in DEBUG mode!!!" )
    #enable debug
    DEFINES += ENABLE_ULOG_DBG
    DEFINES += ENABLE_TPOOL_DBG
}

contains ( DEFINES, 'ULOG_NETLOGGING' ) {
    message("[*] Network Logging: ENABLED")
    QT += network
    HEADERS +=  src/RemoteWriter.h

    SOURCES += 	src/RemoteWriter.cpp
}
else {
    message("[*] Network Logging: DISABLED")
}

contains ( DEFINES, 'ULOG_DBLOGGING' ) {
    message("[*] Db Logging:      ENABLED")
    QT += sql

    HEADERS +=  src/DbWriter.h \
                src/DbHandler.h

    SOURCES += 	src/DbWriter.cpp \
                src/DbHandler.cpp
}
else {
    message("[*] Db Logging: DISABLED")
}
# --------

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


win32-msvc2015{
    message("Using VC++ 2015")
    COMPILER=VC2015
}

message("COMPILER: $$COMPILER")

DSTDIR = $$PWD/last_build/
FINALDIR = $$join(COMPILER,,,_qt-$$QT_VERSION)
DLLPATH = bin/

ios {
    FINALDIR = $$join(FINALDIR,,,_ios)
}

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
    message("Building UniqLogger for Linux / Unix")

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
        message("Building for android")
    }
}



android {
    message("Building UniqLogger for Android")
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
}




# Mac OS X only
macx {
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
    message("Building the library for iOS")
    CONFIG += staticlib

    iphoneos {
        message("Building for a real iPhone device")
        IOSSUFFIX = _iphoneos
    }

    iphonesimulator {
        message("Building for an iPhone simulator (x86)")
        IOSSUFFIX = _iphonesimulator
    }


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
    QMAKE_POST_LINK="mkdir -p $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK+="cp -aP $$DLL $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK+="cp -aP $$DLL $$DSTDIR $$escape_expand(\\n\\t)"
}

message ("Library name: $$DLL")

# ----- Library sources ------

HEADERS += \
    src/Logger.h \
    src/LogWriter.h \
    src/FileWriter.h \
    src/ConsoleWriter.h \
    src/UniqLogger.h \
    src/LogMessage.h \
    src/tpool/nrthreadpool.h \
    src/DummyWriter.h \
    src/bufferofstrings.h

SOURCES += src/Logger.cpp
SOURCES += src/LogWriter.cpp
SOURCES += src/FileWriter.cpp
SOURCES += src/ConsoleWriter.cpp
SOURCES += src/UniqLogger.cpp
SOURCES += src/LogMessage.cpp
SOURCES += src/tpool/nrthreadpool.cpp
SOURCES += src/DummyWriter.cpp
SOURCES += src/bufferofstrings.cpp

INCLUDEPATH += src/tpool
INCLUDEPATH += $$FILECOMPRESSOR_ROOT
INCLUDEPATH += $$MINIZIP_BASE_PATH
INCLUDEPATH += $$ZLIB_BASE_PATH/include/zlib
INCLUDEPATH += $$ZLIB_BASE_PATH/include

unix {
    ## to enable fopen64 on ubunutu 12.04
    DEFINES *= _LARGEFILE64_SOURCE
    message ("[*] linking zlib statically...")
    LIBS += -L$$ZLIB_PATH $$ZLIB_PATH/libz.a
}
win32 {
    message ("[*] linking zlib statically...")
    LIBS += -L$$ZLIB_PATH $$ZLIB_PATH/zlib.lib
    message ("HAVE_UNISTD_H:")
}

QMAKE_CLEAN += -r
QMAKE_CLEAN += $$DLL $$FINALDIR $$DSTDIR/*
QMAKE_DISTCLEAN += $$QMAKE_CLEAN

message(" ==== End of UniqLogger QMake build process ==== ")

DISTFILES += \
    depspath.pri.sample \
    configs/buildbot_win7.pri \
    ci/bbot_iqac_agent_config.pri

SUBDIRS += \
    configs/buildbot-uniqlogger.pro
