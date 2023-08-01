UNQLPATH = $$PWD/../lib

INCLUDEPATH += $$UNQLPATH/src

CONFIG(release, debug|release) {
    LIBS += -lUniqLogger
}

win32 {
    CONFIG(release, debug|release) {
        LIBS += -L$$UNQLPATH/last_build -lUniqLogger1
    }

    CONFIG(debug, debug|release) {
        LIBS += -L$$UNQLPATH/last_build -lUniqLoggerd1
    }
}


message ("Libs: $$LIBS")

unix:!macx {
    CONFIG(debug, debug|release) {
        LIBS +=  -lUniqLogger_d
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

