#!/bin/bash

set -e

QT_SYSTEM_PATH="/usr/local/Trolltech/Qt-4.8.1/lib"
DEPLOY_DIR="deployable_binaries"

install_name_tool -change ${QT_SYSTEM_PATH}/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.4.dylib bin/libUniqLogger.0.1.3.dylib
install_name_tool -change ${QT_SYSTEM_PATH}/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.4.dylib bin/libUniqLogger.0.1.3.dylib

[ ! -d ${DEPLOY_DIR} ] && mkdir ${DEPLOY_DIR} 
cp -rf bin/libUniqLogger.0.dylib ${DEPLOY_DIR}/ 

