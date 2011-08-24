
SRCROOT := ../..
COMPILER_NAME := bcc
OBJ_EXT := obj

STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

INCLUDES += -I${STLPORT_INCLUDE_DIR}

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

LDSEARCH = -L${STLPORT_LIB_DIR}

