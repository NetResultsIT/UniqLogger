##############################
#  UniqLogger Configuration  #
##############################

VERSION = 0.2.2

# --- Please check that the config file reflects your desired build options
include (config.pri)

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

message("COMPILER: $$COMPILER")

DSTDIR = $$PWD/last_build/
FINALDIR = $$join(COMPILER,,,_qt-$$QT_VERSION)
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
		QMAKE_CXXFLAGS_RELEASE += /Zi 
		QMAKE_LFLAGS_RELEASE += /DEBUG
		QMAKE_LFLAGS_RELEASE += /OPT:REF
		QMAKE_LFLAGS_RELEASE += /OPT:ICF
        DLL = $$join(TARGET,,release\\,$$MYVER)
    }
    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$FINALDIR\" $$escape_expand(\\n\\t)"
    for(ext, WINEXT):QMAKE_POST_LINK+="$$QMAKE_COPY $$join(DLL,,,.$${ext}) \"$$DSTDIR\" $$escape_expand(\\n\\t)"
}

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

unix:!macx {
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
    #message ("unix!macx DLL $$DLL DLLPATH $$DLLPATH TARGET $$TARGET")
}

macx {
    CONFIG(debug, debug|release) {
        TARGET = $$join(TARGET,,,_debug)
        DLL = $$join(TARGET,,lib,.*dylib)
	DLLPATH=$$join(DLLPATH,,debug/,)
	TARGET = $$join(TARGET,,$$DLLPATH,)
	DLL=$$join(DLL,,$$DLLPATH,)
    }
    CONFIG(release, debug|release) {
        #DLL = $$join(TARGET,,release/,.dylib*)
	TARGET = $$join(TARGET,,,)
        DLL = $$join(TARGET,,lib,.*dylib)
	DLLPATH=$$join(DLLPATH,,release/,)
	TARGET = $$join(TARGET,,$$DLLPATH,)
	DLL=$$join(DLL,,$$DLLPATH,)
    }
    #message ("macx DLL $$DLL DLLPATH $$DLLPATH TARGET $$TARGET")
}

unix {
    QMAKE_POST_LINK="mkdir -p $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK+="cp -aP $$DLL $$FINALDIR $$escape_expand(\\n\\t)"
    QMAKE_POST_LINK+="cp -aP $$DLL $$DSTDIR $$escape_expand(\\n\\t)"
}


message ("Library name: $$TARGET$$MYVER")

# ----- Library sources ------

HEADERS += \
           src/Logger.h \
           src/LogWriter.h \
           src/FileWriter.h \
           src/ConsoleWriter.h \
           src/UniqLogger.h \
           src/LogMessage.h \
           src/DummyWriter.h

SOURCES += \
           src/Logger.cpp \
           src/LogWriter.cpp \
           src/FileWriter.cpp \
           src/ConsoleWriter.cpp \
           src/UniqLogger.cpp \
           src/LogMessage.cpp \
           src/DummyWriter.cpp




QMAKE_CLEAN += -r
QMAKE_CLEAN += $$DLL $$FINALDIR $$DSTDIR/*
QMAKE_DISTCLEAN += $$QMAKE_CLEAN

message(" ==== End of QMake build process ==== ")
