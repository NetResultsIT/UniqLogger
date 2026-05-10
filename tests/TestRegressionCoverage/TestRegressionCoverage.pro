QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

SOURCES += tst_testregressioncoverage.cpp

include(../tests_common.pri)

exists(../../lib/config.pri) {
    include(../../lib/config.pri)
}

contains(DEFINES, ENABLE_UNQL_NETLOG) {
    QT += network
}
