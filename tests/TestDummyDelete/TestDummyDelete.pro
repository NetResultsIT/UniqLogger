QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_testdeletedummy.cpp


include(../tests_common.pri)

#on windows the symbols of internal classes aare not exposed in the unqilogger dll so we need to compile single classes here
win32 {

include("$$UNQLPATH/src/ext/filecompressor/src/nrFileCompressor.pri")

HEADERS += $$UNQLPATH/src/LogWriter.h

SOURCES += $$UNQLPATH/src/LogWriter.cpp\
$$UNQLPATH/src/LogMessage.cpp \
}
