#!/bin/bash

HOST_SYSTEM=`uname -s`
COMPONENT=brubeck_runtime

while [ -z $FROGNAME ] ; do
    read -p "Please enter your jFrog username: " FROGNAME
done

rm -f ${COMPONENT}.${HOST_SYSTEM}.tar.gz
curl -u ${FROGNAME} -O "http://jfrog.invidi.com:80/artifactory/rwi-files-local/latest/${COMPONENT}.${HOST_SYSTEM}.tar.gz"
tar xvzf ${COMPONENT}.${HOST_SYSTEM}.tar.gz
pushd ${COMPONENT}
./install.sh
popd
rm -rf ${COMPONENT}
rm -f ${COMPONENT}.${HOST_SYSTEM}.tar.gz



