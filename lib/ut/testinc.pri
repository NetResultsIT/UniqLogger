QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../src


LIBS += -L$$PWD/../last_build

unix:!macx {
SUFFIX = "d"
}

macx {
SUFFIX = "debug"
}

CONFIG(debug, debug|release) {
    LIBS += -lUniqLogger_$$SUFFIX
} else {
    LIBS += -lUniqLogger
}
