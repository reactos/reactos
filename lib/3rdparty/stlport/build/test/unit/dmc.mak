# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>

SRCROOT := ../..
COMPILER_NAME := dmc
OBJ_EXT := obj

STLPORT_DIR := ../../..

include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak


INCLUDES += -I$(STLPORT_INCLUDE_DIR)

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
dbg-static:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-static:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

