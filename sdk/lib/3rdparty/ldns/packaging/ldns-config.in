#!/bin/sh

prefix="@prefix@"
exec_prefix="@exec_prefix@"
VERSION="@PACKAGE_VERSION@"
CFLAGS="@CFLAGS@"
CPPFLAGS="@CPPFLAGS@ @LIBSSL_CPPFLAGS@"
LDFLAGS="@LDFLAGS@ @LIBSSL_LDFLAGS@"
PYTHON_CPPFLAGS="@PYTHON_CPPFLAGS@"
PYTHON_LDFLAGS="@PYTHON_LDFLAGS@"
LIBS="@LIBS@ @LIBSSL_LIBS@"
LIBDIR="@libdir@"
INCLUDEDIR="@includedir@"
LIBVERSION="@VERSION_INFO@"


for arg in $@
do
    if [ $arg = "--cflags" ]
    then
        echo "-I${INCLUDEDIR}"
    fi
    if [ $arg = "--python-cflags" ]
    then
        echo "${PYTHON_CPPFLAGS} -I${INCLUDEDIR}"
    fi
    if [ $arg = "--libs" ]
    then
        echo "${LDFLAGS} -L${LIBDIR} ${LIBS} -lldns"
    fi
    if [ $arg = "--python-libs" ]
    then
        echo "${LDFLAGS} ${PYTHON_LDFLAGS} -L${LIBDIR} ${LIBS} -lldns"
    fi
    if [ $arg = "-h" ] || [ $arg = "--help" ]
    then
        echo "Usage: $0 [--cflags] [--python-cflags] [--libs] [--python-libs] [--version]"
    fi
    if [ $arg = "--version" ]
    then
        echo "${VERSION}"
    fi
    if [ $arg = "--libversion" ]
    then
        echo "${LIBVERSION}"
    fi
done
