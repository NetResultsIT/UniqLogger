QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../src

SOURCES +=  tst_writerconfig.cpp

LIBS += -L$$PWD/../../last_build

CONFIG(debug, debug|release) {
    LIBS += -lUniqLogger_d
} else {
    LIBS += -lUniqLogger
}
