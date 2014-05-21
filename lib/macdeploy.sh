#!/bin/bash

ULVER=0.2.0

# Do not modify below here
set -e

QT_SYSTEM_PATH="/usr/local/Trolltech/Qt-4.8.1/lib"
DEPLOY_DIR="deployable_binaries"

if [ -e release/bin/libUniqLogger.0.2.0.dylib ] ; then
	install_name_tool -change ${QT_SYSTEM_PATH}/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.4.dylib release/bin/libUniqLogger.0.2.0.dylib
	install_name_tool -change ${QT_SYSTEM_PATH}/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.4.dylib release/bin/libUniqLogger.0.2.0.dylib
fi

if [ -e debug/bin/libUniqLogger_debug.0.2.0.dylib ] ; then
	install_name_tool -change ${QT_SYSTEM_PATH}/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.4.dylib debug/bin/libUniqLogger_debug.0.2.0.dylib
	install_name_tool -change ${QT_SYSTEM_PATH}/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.4.dylib debug/bin/libUniqLogger_debug.0.2.0.dylib
fi

[ ! -d ${DEPLOY_DIR} ] && mkdir ${DEPLOY_DIR} 
cp -rf release/bin/libUniqLogger.0.dylib ${DEPLOY_DIR}/ 
cp -rf debug/bin/libUniqLogger_debug.0.dylib ${DEPLOY_DIR}/ 

