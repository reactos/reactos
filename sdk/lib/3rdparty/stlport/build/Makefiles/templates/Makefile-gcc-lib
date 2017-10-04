# -*- Makefile -*- Time-stamp: <03/07/09 18:08:47 ptr>
# $Id$

SRCROOT := ../..
COMPILER_NAME := gcc

STLPORT_LIB_DIR = /export/home/ptr/STLport.lab/STLport/lib
STLPORT_INCLUDE_DIR = /export/home/ptr/STLport.lab/STLport/stlport
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak


INCLUDES += -I$(SRCROOT)/include -I$(STLPORT_INCLUDE_DIR)

ifeq ($(OSNAME),linux)
release-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
stldbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
dbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
endif

ifeq ($(OSNAME),sunos)
release-shared : LDLIBS = -lstlport_gcc -lrt
stldbg-shared  : LDLIBS = -lstlport_gcc_stldebug -lrt
dbg-shared     : LDLIBS = -lstlport_gcc -lrt
else
release-shared : LDLIBS = -lstlport_gcc
stldbg-shared  : LDLIBS = -lstlport_gcc_stldebug
dbg-shared     : LDLIBS = -lstlport_gcc
endif

