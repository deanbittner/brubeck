#!/bin/bash

HOST_SYSTEM=`uname -s`
COMPONENT=brubeck_runtime
pushd ..

if [ "${1}" == "local" ] && [ -f /tmp/${COMPONENT}.${HOST_SYSTEM}.tar.gz ] ; then
    cp /tmp/${COMPONENT}.${HOST_SYSTEM}.tar.gz .
else
    rm -f ${COMPONENT}.${HOST_SYSTEM}.tar.gz
    wget http://trouble.bottorrent.net/static/RWI/runtime/${COMPONENT}.${HOST_SYSTEM}.tar.gz
fi

tar xvzf ${COMPONENT}.${HOST_SYSTEM}.tar.gz

popd

