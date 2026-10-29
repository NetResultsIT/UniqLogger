# Qt / Apple Clang (macOS 26.4 SDK) workaround for QTBUG-145239.
#
# Affected Qt headers call __yield() from QtCore/qyieldcpu.h without declaring
# it, so the arm64 slice fails with:
#   error: implicitly declaring library function '__yield'
# Upstream fixed it by checking __builtin_arm_yield first (Qt 6.10 branch, 6.12.0);
# the 6.8 branch is not fixed. So force-include our wrapper only when the Qt in use
# still has the old header. The wrapper is a no-op on non-ARM targets.

QT_YIELD_HEADER = $$[QT_INSTALL_LIBS]/QtCore.framework/Headers/qyieldcpu.h
!exists($$QT_YIELD_HEADER): QT_YIELD_HEADER = $$[QT_INSTALL_HEADERS]/QtCore/qyieldcpu.h

QT_YIELD_FIXED =
exists($$QT_YIELD_HEADER) {
    QT_YIELD_LINES = $$cat($$QT_YIELD_HEADER, lines)
    # fixed header: the cascade starts with an "if" (not "elif") on __builtin_arm_yield.
    # A literal hash cannot be used in a qmake string, hence the "." in the regex.
    QT_YIELD_FIXED = $$find(QT_YIELD_LINES, "^.if __has_builtin\\(__builtin_arm_yield\\)")
}

isEmpty(QT_YIELD_FIXED) {
    message("Qt qyieldcpu.h needs the __yield() workaround, force-including it")
    QMAKE_CXXFLAGS += -include $$PWD/src/qt_arm_acle_workaround.h
    QMAKE_CFLAGS   += -include $$PWD/src/qt_arm_acle_workaround.h
} else {
    message("Qt qyieldcpu.h already fixed upstream, no __yield() workaround needed")
}
