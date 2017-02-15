#!/bin/bash

set -x
NAME=brubeck
VERSION=1.0.0
BRUBECK_SRCNAME=${NAME}-${VERSION}
BRUBECK_ARCHIVE=${BRUBECK_SRCNAME}.tar.gz

[ ! -f ../brubeck ] && echo "Need to build brubeck first." && exit 1

mkdir -p ~/rpmbuild/{RPMS,SRPMS,BUILD,SOURCES,SPECS,tmp}
cat <<EOF >~/.rpmmacros
%_topdir  %(echo $HOME)/rpmbuild
%_tmppath  %{_topdir}/tmp
EOF

rm -rf ${BRUBECK_ARCHIVE}
mkdir -p ${BRUBECK_SRCNAME}/usr/local/sbin
cp ../brubeck ${BRUBECK_SRCNAME}/usr/local/sbin/brubeck
mkdir -p ${BRUBECK_SRCNAME}/etc/init.d
cp brubeck.init ${BRUBECK_SRCNAME}/etc/init.d/brubeck
mkdir -p ${BRUBECK_SRCNAME}/etc/brubeck
cp config.json ${BRUBECK_SRCNAME}/etc/brubeck/config.json

tar cvzf ${BRUBECK_ARCHIVE} ${BRUBECK_SRCNAME}/etc/init.d/brubeck ${BRUBECK_SRCNAME}/etc/brubeck/config.json ${BRUBECK_SRCNAME}/usr/local/sbin/brubeck
mv ${BRUBECK_ARCHIVE} ~/rpmbuild/SOURCES/.

rm -rf ${BRUBECK_SRCNAME}

# rpmlint brubeck.spec
rpmbuild -ba brubeck.spec

# test
#rpm -e packagename 
