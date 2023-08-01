QT += core network testlib
QT -= gui

CONFIG += c++11

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app


SOURCES +=  tst_testmessagescompression.cpp


include(../tests_common.pri)
message($$INCLUDEPATH)
