#!/bin/bash

ULVER=0.2.0
QTVER=5.2.1

# Do not modify below here
set -e

#Qt4
QT_SYSTEM_PATH="/usr/local/Trolltech/Qt-4.8.1/lib"
#Qt5
QT_SYSTEM_PATH=~/Qt/${QTVER}/clang_64/lib

SRCDIR=last_build
DEPLOY_DIR="deployable_binaries"

[ ! -d ${DEPLOY_DIR} ] && mkdir ${DEPLOY_DIR}
rm -rf ${DEPLOY_DIR}/*
cp -aP ${SRCDIR}/libUniqLogger.*dylib ${DEPLOY_DIR}/
#cp -rf ${SRCDIR}/libUniqLogger_debug.0.dylib ${DEPLOY_DIR}/

if [ -e ${SRCDIR}/libUniqLogger.${ULVER}.dylib ] ; then
    #install_name_tool -change ${QT_SYSTEM_PATH}/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.4.dylib ${SRCDIR}/libUniqLogger.${ULVER}.dylib
    #install_name_tool -change ${QT_SYSTEM_PATH}/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.4.dylib ${SRCDIR}/libUniqLogger.${ULVER}.dylib
    install_name_tool -change ${QT_SYSTEM_PATH}/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ${DEPLOY_DIR}/libUniqLogger.${ULVER}.dylib
    install_name_tool -change ${QT_SYSTEM_PATH}/QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork ${DEPLOY_DIR}/libUniqLogger.${ULVER}.dylib
fi

#if [ -e ${SRCDIR}/libUniqLogger_debug.${ULVER}.dylib ] ; then
#	install_name_tool -change ${QT_SYSTEM_PATH}/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.4.dylib ${DEPLOY_DIR}/libUniqLogger_debug.${ULVER}.dylib
#	install_name_tool -change ${QT_SYSTEM_PATH}/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.4.dylib ${DEPLOY_DIR}/libUniqLogger_debug.${ULVER}.dylib
#fi



