# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>

SRCROOT := ..
COMPILER_NAME := bcc

STLPORT_INCLUDE_DIR = ../../stlport
include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

ifneq ($(OSNAME),linux)
OBJ_EXT := obj
ifndef INCLUDE
$(error Missing INCLUDE environment variable definition. Please see doc/README.borland \
for instructions about how to prepare Borland compiler to build STLport libraries.)
endif
else
DEFS += -D_GNU_SOURCE
GCC_VERSION := $(shell gcc -dumpversion)
DEFS += -DGCC_VERSION=$(GCC_VERSION)
endif

INCLUDES += -I$(STLPORT_INCLUDE_DIR)

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

