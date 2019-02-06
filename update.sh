#!/bin/bash

HOST_SYSTEM=`uname -s`
COMPONENT=brubeck
pushd ..

if [ "${1}" == "local" ] && [ -f /tmp/runtime.${HOST_SYSTEM}.tar.gz ] ; then
    cp /tmp/${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz .
else
    rm -f ${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz
    wget http://trouble.bottorrent.net/static/RWI/runtime/${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz
fi

tar xvzf ${COMPONENT}_runtime.${HOST_SYSTEM}.tar.gz

popd

