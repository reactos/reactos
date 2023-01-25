#!/bin/ksh
#
# $Id$


PREFIX=/opt/ldns
OPENSSL=/usr/sfw
SUDO=sudo

MAKE_PROGRAM=gmake
MAKE_ARGS="-j 4"

OBJ32=obj32
OBJ64=obj64

SRCDIR=`pwd`


test -d $OBJ32 && $SUDO rm -fr $OBJ32
mkdir $OBJ32

export CFLAGS=""
export LDFLAGS="-L${OPENSSL}/lib -R${OPENSSL}/lib"

(cd $OBJ32; \
${SRCDIR}/configure --with-ssl=${OPENSSL} --prefix=${PREFIX} --libdir=${PREFIX}/lib; \
$MAKE_PROGRAM $MAKE_ARGS)

if [ `isainfo -k` = amd64 ]; then
	test -d $OBJ64 && $SUDO rm -fr $OBJ64
	mkdir $OBJ64

	export CFLAGS="-m64"
	export LDFLAGS="-L${OPENSSL}/lib/amd64 -R${OPENSSL}/lib/amd64"

	(cd $OBJ64; \
	${SRCDIR}/configure --with-ssl=${OPENSSL} --prefix=${PREFIX} --libdir=${PREFIX}/lib/amd64; \
	$MAKE_PROGRAM $MAKE_ARGS)
fi

# optionally install
#
if [ x$1 = xinstall ]; then
	(cd $OBJ32; $SUDO $MAKE_PROGRAM install-h)
	(cd $OBJ32; $SUDO $MAKE_PROGRAM install-doc)
	(cd $OBJ32; $SUDO $MAKE_PROGRAM install-lib)
	if [ `isainfo -k` = amd64 ]; then
		(cd $OBJ64; $SUDO $MAKE_PROGRAM install-lib)
	fi
fi
