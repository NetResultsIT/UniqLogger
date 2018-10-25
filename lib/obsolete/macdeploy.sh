#!/bin/bash

ULVER=0.3.3
QTVER=5.6.1

# Do not modify below here
set -e

#Qt5
QT_SYSTEM_PATH=/opt/Qt/${QTVER}/clang_64/lib

SRCDIR=last_build
DEPLOY_DIR="deployable_binaries"

[ ! -d ${DEPLOY_DIR} ] && mkdir ${DEPLOY_DIR}
rm -rf ${DEPLOY_DIR}/*
cp -aP ${SRCDIR}/libUniqLogger.*dylib ${DEPLOY_DIR}/
#cp -rf ${SRCDIR}/libUniqLogger_debug.0.dylib ${DEPLOY_DIR}/

if [ -e ${SRCDIR}/libUniqLogger.${ULVER}.dylib ] ; then
    install_name_tool -change @rpath/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ${DEPLOY_DIR}/libUniqLogger.${ULVER}.dylib
    install_name_tool -change @rpath/QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork ${DEPLOY_DIR}/libUniqLogger.${ULVER}.dylib
    install_name_tool -change @rpath/QtSql.framework/Versions/5/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/5/QtSql ${DEPLOY_DIR}/libUniqLogger.${ULVER}.dylib
fi
