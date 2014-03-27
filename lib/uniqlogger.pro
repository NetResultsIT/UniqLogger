##################################
# Uniqlogger.pro
#

QT -= gui

TARGET = bin/UniqLogger

ULOG_VERSION = 0.2.0

CONFIG += warn_on dll debug_and_release
CONFIG -= flat

DEFINES -= UNICODE 

# If you want to enable network logging uncomment the following line
DEFINES += ULOG_NETLOGGING

# If you want to enable db logging uncomment the following line
#DEFINES += ULOG_DBLOGGING

# If you want to enable debug statements in the UniqLogger library uncomment the following line
#DEFINES += ULOGDBG

# ---- DO NOT CHANGE *ANYTHING* BELOW THIS LINE ---- #

TEMPLATE = lib

contains ( DEFINES, ULOG_NETLOGGING ) {
    message ( "[*] Network Logging support ENABLED" )
}
contains ( DEFINES, ULOG_DBLOGGING ) {
    message ( "[*] Db Logging support ENABLED" )
}
contains ( DEFINES, ULOGDBG ) {
    message ( "[*] Library debug mode ENABLED" )
}

win32 {
	CONFIG += flat
     contains(IDE,VS) {
      TEMPLATE = vclib
     }
  	DEFINES += _CRT_SECURE_NO_DEPRECATE ULOG_LIB_EXPORTS

  	LIBS += ws2_32.lib \
		Psapi.lib 
}

unix {
	OBJECTS_DIR = build
	MOC_DIR = build
	VERSION = $$ULOG_VERSION
	QMAKE_CFLAGS += -g
	QMAKE_CXXFLAGS += -g
	QMAKE_LFLAGS += -g
}

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
