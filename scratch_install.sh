#!/bin/bash

HOST_SYSTEM=`uname -s`
COMPONENT=brubeck

rm -f ${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz
wget http://trouble.bottorrent.net/static/RWI/runtime/${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz
tar xvzf ${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz
pushd brubeck_runtime
./install.sh
popd
rm -rf brubeck_runtime
rm -f ${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz



