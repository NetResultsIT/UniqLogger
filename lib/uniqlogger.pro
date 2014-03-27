##################
# Uniqlogger.pro #
##################

QT -= gui

TARGET = bin/UniqLogger
VERSION = 0.2.0


# If you want to enable network logging uncomment the following line
DEFINES += ULOG_NETLOGGING

# If you want to enable db logging uncomment the following line
#DEFINES += ULOG_DBLOGGING

# If you want to enable debug statements in the UniqLogger library uncomment the following line
#DEFINES += ULOGDBG

# ---- DO NOT CHANGE *ANYTHING* BELOW THIS LINE ---- #

CONFIG += warn_on dll
CONFIG -= flat

DEFINES -= UNICODE

TEMPLATE = lib

#this should work with Qt5, on Qt4 we do it manually
MYVER = $$split($$VERSION, .)
MYVER = 0
message("$$QT_VERSION $$VERSION")

MDCMD = mkdir

contains ( DEFINES, ULOGDBG ) {
    message ( "[*] Library debug mode ENABLED" )
}

win32 {
    CONFIG += flat
    contains(IDE,VS) {
      TEMPLATE = vclib
    }
    DEFINES += _CRT_SECURE_NO_DEPRECATE ULOG_LIB_EXPORTS

    MDCMD = md

    LIBS += ws2_32.lib \
            Psapi.lib

    CONFIG(debug, debug|release) {
        TARGET = bin/UniqLoggerd
        QMAKE_POST_LINK="md ..\\lib\\debug\\qt-$$QT_VERSION\\ $$escape_expand(\n\t)"
        QMAKE_POST_LINK+="copy .\\debug\\bin\\UniqLoggerd.dll ..\\lib\\debug\\qt-$$QT_VERSION\\ /y$$escape_expand(\n\t)"
        QMAKE_POST_LINK+="copy .\\debug\\bin\\UniqLoggerd.lib ..\\lib\\debug\\qt-$$QT_VERSION\\ /y$$escape_expand(\n\t)"
    }

    CONFIG(release, debug|release) {
        QMAKE_POST_LINK="md ..\\lib\\release\\qt-$$QT_VERSION\\ $$escape_expand(\n\t)"
        QMAKE_POST_LINK+="copy .\\release\\bin\\UniqLogger.* ..\\lib\\release\\qt-$$QT_VERSION\\ /y"
    }
}

unix {
    OBJECTS_DIR = build
    MOC_DIR = build
    #VERSION = $$ULOG_VERSION
    CONFIG(debug, debug|release) {
        QMAKE_CFLAGS += -g
        QMAKE_CXXFLAGS += -g
        QMAKE_LFLAGS += -g
    }
}

unix:!macx {
    CONFIG(debug, debug|release) {
        TARGET = bin/UniqLogger_d
        QMAKE_POST_LINK="mkdir -p ../lib/debug/qt-$$QT_VERSION/ $$escape_expand(\n\t)"
        QMAKE_POST_LINK+="cp -aP ./bin/libUniqLogger_d.so* ../lib/debug/qt-$$QT_VERSION/ $$escape_expand(\n\t)"
    }

    CONFIG(release, debug|release) {
        QMAKE_POST_LINK="mkdir -p ../lib/release/qt-$$QT_VERSION/ $$escape_expand(\n\t)"
        QMAKE_POST_LINK+="cp -aP ./bin/libUniqLogger.so* ../lib/release/qt-$$QT_VERSION/"
    }
}

macx {
    CONFIG(debug, debug|release) {
        TARGET = bin/UniqLogger_debug
    }
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
