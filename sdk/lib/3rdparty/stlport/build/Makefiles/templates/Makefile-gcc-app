# -*- Makefile -*- Time-stamp: <03/07/09 18:08:47 ptr>
# $Id$

SRCROOT := ../../..
COMPILER_NAME := gcc

include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak


INCLUDES += -I$(SRCROOT)/include -I$(STLPORT_INCLUDE_DIR)

release-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -L${CoMT_LIB_DIR}
stldbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -L${CoMT_LIB_DIR_STLDBG}
dbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -L${CoMT_LIB_DIR_DBG}

ifeq ($(OSNAME),sunos)
release-shared : LDLIBS = -lstlport_gcc -lrt
stldbg-shared  : LDLIBS = -lstlport_gcc_stldebug -lrt
dbg-shared     : LDLIBS = -lstlport_gcc -lrt
else
release-shared : LDLIBS = -lstlport_gcc -lxmt_gcc
stldbg-shared  : LDLIBS = -lstlport_gcc_stldebug -lxmt_gcc_stl-g
dbg-shared     : LDLIBS = -lstlport_gcc -lxmt_gcc-g
endif

