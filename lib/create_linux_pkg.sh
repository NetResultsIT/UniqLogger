#!/bin/bash

set -e

if [ -z $1 ] ; then
	echo "Usage $0 <revision>"
	exit 1
fi

REV=${1}
TARGET=./lib_uniqlogger
#TARGET_LIBS=${TARGET}/libs

rm -rf ${TARGET}
mkdir ${TARGET}
rm -rf uniqlogger*.tar.gz

# copy binary
cp -av bin/* ${TARGET}

# rm svn directory
find ${TAREGT} -iname .svn | xargs rm -rf 
# create tag archive
PNAME="uniqlogger_$(uname -i)-r${REV}.tar.gz"
tar czf ${PNAME} ${TARGET}

