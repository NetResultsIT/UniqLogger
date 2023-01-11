QT += core network testlib
QT -= gui

CONFIG += c++11

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app


UNQLPATH = $$PWD/../../lib

INCLUDEPATH += $$UNQLPATH/src

SOURCES += \
    tst_FileWriterRotations.cpp

HEADERS += \
    tst_FileWriterRotations.h

LIBS += -L$$UNQLPATH/last_build

CONFIG(release, debug|release) {
    LIBS += -lUniqLogger1
}


message ("Libs: $$LIBS")

unix:!macx {
    CONFIG(debug, debug|release) {
        LIBS += -lUniqLogger_d
    }
}

macx {
    CONFIG(debug, debug|release) {
        LIBS += -lUniqLogger_debug
    }

    LIBS += -L/$$UNQLPATH/last_build

    QMAKE_LFLAGS += \
                -Wl,-rpath $$UNQLPATH/last_build
}


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

