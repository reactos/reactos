# -*- Makefile -*- Time-stamp: <08/06/06 10:50:42 yeti>

SRCROOT := ..
COMPILER_NAME := gcc
STLP_BUILD := 1

STLPORT_INCLUDE_DIR = ../../stlport
HEADERS_BASE = ${STLPORT_INCLUDE_DIR}

include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak
include ${SRCROOT}/Makefiles/gmake/subdirs.mak

ifeq ($(OSNAME),linux)
DEFS += -D_GNU_SOURCE
endif

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

SUBDIRS := ${SRCROOT}/test/unit

check:	all
	+$(call doinsubdirs,${SUBDIRS})
