# -*- Makefile -*- Time-stamp: <05/03/10 17:51:53 ptr>

SRCROOT := ../..
COMPILER_NAME := bcc

STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

INCLUDES += -I${STLPORT_INCLUDE_DIR}

ifneq ($(OSNAME),linux)
OBJ_EXT := obj
else
DEFS += -D_GNU_SOURCE
GCC_VERSION = $(shell gcc -dumpversion)
DEFS += -DGCC_VERSION=$(GCC_VERSION)
endif

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
dbg-static:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-static:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

LDSEARCH = -L${STLPORT_LIB_DIR}

