# -*- Makefile -*- Time-stamp: <05/03/10 17:51:53 ptr>

SRCROOT := ../..
COMPILER_NAME := gcc

ALL_TAGS := release-shared dbg-shared stldbg-shared
STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

INCLUDES += -I${STLPORT_INCLUDE_DIR}
DEFS += -D_STLP_NO_CUSTOM_IO

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

ifndef TARGET_OS
release-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -Wl,-R${STLPORT_LIB_DIR}
dbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -Wl,-R${STLPORT_LIB_DIR}
stldbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR} -Wl,-R${STLPORT_LIB_DIR}
else
release-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
dbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
stldbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
endif

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 

