# Time-stamp: <08/02/28 10:25:46 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifndef _FORCE_CXX
CXX := c++
else
CXX := ${_FORCE_CXX}
endif

ifndef _FORCE_CC
CC := gcc
else
CC := ${_FORCE_CC}
endif

ifeq ($(OSNAME), cygming)
RC := windres
endif

ifdef TARGET_OS
CXX := ${TARGET_OS}-${CXX}
CC := ${TARGET_OS}-${CC}
AS := ${TARGET_OS}-${AS}
endif

CXX_VERSION := $(shell ${CXX} -dumpversion)
CXX_VERSION_MAJOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$1; }')
CXX_VERSION_MINOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$2; }')
CXX_VERSION_PATCH := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$3; }')

# Check that we need option -fuse-cxa-atexit for compiler
_CXA_ATEXIT := $(shell ${CXX} -v 2>&1 | grep -q -e "--enable-__cxa_atexit" || echo "-fuse-cxa-atexit")

ifeq ($(OSNAME), darwin)
# This is to differentiate Apple-builded compiler from original
# GNU compiler (it has different behaviour)
ifneq ("$(shell ${CXX} -v 2>&1 | grep Apple)", "")
GCC_APPLE_CC := 1
endif
endif

DEFS ?=
OPT ?=

ifdef WITHOUT_STLPORT
INCLUDES =
else
INCLUDES = -I${STLPORT_INCLUDE_DIR}
endif

ifdef BOOST_INCLUDE_DIR
INCLUDES += -I${BOOST_INCLUDE_DIR}
endif

ifeq ($(OSNAME), cygming)
ifeq ($(OSREALNAME), mingw)
# MinGW has problem with /usr/local reference in gcc or linker command line so
# we use a local install for this platform.
BASE_INSTALL_DIR ?= ${STLPORT_DIR}
endif
endif

OUTPUT_OPTION = -o $@
LINK_OUTPUT_OPTION = ${OUTPUT_OPTION}
CPPFLAGS = $(DEFS) $(INCLUDES)

ifdef WITHOUT_RTTI
# -fno-rtti shouldn't be pass to the C compiler, we cannot use OPT so we add it
# directly to the compiler command name.
CXX += -fno-rtti
ifdef STLP_BUILD
# gcc do not define any macro to signal that there is no rtti support:
DEFS += -D_STLP_NO_RTTI
endif
endif

ifeq ($(OSNAME), cygming)
WINVER ?= 0x0501
RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} --output-format coff -DCOMP=gcc
release-shared : RCFLAGS += -DBUILD_INFOS=-O2
dbg-shared : RCFLAGS += -DBUILD=g -DBUILD_INFOS=-g
stldbg-shared : RCFLAGS += -DBUILD=stlg -DBUILD_INFOS="-g -D_STLP_DEBUG"
RC_OUTPUT_OPTION = -o $@
CXXFLAGS = -Wall -Wsign-promo -Wcast-qual -fexceptions
ifndef WITHOUT_THREAD
ifeq ($(OSREALNAME), mingw)
CCFLAGS += -mthreads
CFLAGS += -mthreads
CXXFLAGS += -mthreads
ifeq ($(CXX_VERSION_MAJOR),2)
CCFLAGS += -fvtable-thunks
CFLAGS += -fvtable-thunks
CXXFLAGS += -fvtable-thunks
endif
else
ifneq (,$(findstring no-cygwin,$(EXTRA_CXXFLAGS)))
CCFLAGS += -mthreads
CFLAGS += -mthreads
CXXFLAGS += -mthreads
else
DEFS += -D_REENTRANT
endif
endif
endif
CCFLAGS += $(OPT)
CFLAGS += $(OPT)
CXXFLAGS += $(OPT)
COMPILE.rc = $(RC) $(RCFLAGS)
release-static : DEFS += -D_STLP_USE_STATIC_LIB
dbg-static : DEFS += -D_STLP_USE_STATIC_LIB
stldbg-static : DEFS += -D_STLP_USE_STATIC_LIB
ifeq ($(OSREALNAME), mingw)
dbg-shared : DEFS += -D_DEBUG
stldbg-shared : DEFS += -D_DEBUG
dbg-static : DEFS += -D_DEBUG
stldbg-static : DEFS += -D_DEBUG
DEFS += -DWINVER=${WINVER}
else
# When using the -mno-cygwin option we need to take into account WINVER.
# As there is no DEFS for C compiler and an other for C++ we use CFLAGS
# and CXXFLAGS
ifdef EXTRA_CXXFLAGS
ifneq (,$(findstring no-cygwin,$(EXTRA_CXXFLAGS)))
CXXFLAGS += -DWINVER=${WINVER}
endif
endif
ifdef EXTRA_CFLAGS
ifneq (,$(findstring no-cygwin,$(EXTRA_CFLAGS)))
CFLAGS += -DWINVER=${WINVER}
endif
endif
endif
endif

ifndef WITHOUT_THREAD
PTHREAD := -pthread
else
PTHREAD :=
endif

ifeq ($(OSNAME),sunos)
ifndef WITHOUT_THREAD
PTHREADS := -pthreads
else
PTHREADS :=
endif

CCFLAGS = $(PTHREADS) $(OPT)
CFLAGS = $(PTHREADS) $(OPT)
# CXXFLAGS = $(PTHREADS) -nostdinc++ -fexceptions $(OPT)
CXXFLAGS = $(PTHREADS) -fexceptions $(OPT)
endif

ifeq ($(OSNAME),linux)
CCFLAGS = $(PTHREAD) $(OPT)
CFLAGS = $(PTHREAD) $(OPT)
# CXXFLAGS = $(PTHREAD) -nostdinc++ -fexceptions $(OPT)
CXXFLAGS = $(PTHREAD) -fexceptions $(OPT)
endif

ifeq ($(OSNAME),openbsd)
CCFLAGS = $(PTHREAD) $(OPT)
CFLAGS = $(PTHREAD) $(OPT)
# CXXFLAGS = $(PTHREAD) -nostdinc++ -fexceptions $(OPT)
CXXFLAGS = $(PTHREAD) -fexceptions $(OPT)
endif

ifeq ($(OSNAME),freebsd)
CCFLAGS = $(PTHREAD) $(OPT)
CFLAGS = $(PTHREAD) $(OPT)
ifndef WITHOUT_THREAD
DEFS += -D_REENTRANT
endif
# CXXFLAGS = $(PTHREAD) -nostdinc++ -fexceptions $(OPT)
CXXFLAGS = $(PTHREAD) -fexceptions $(OPT)
endif

ifeq ($(OSNAME),darwin)
CCFLAGS = $(OPT)
CFLAGS = $(OPT)
ifndef WITHOUT_THREAD
DEFS += -D_REENTRANT
endif
CXXFLAGS = -fexceptions $(OPT)
release-shared : CXXFLAGS += -dynamic
dbg-shared : CXXFLAGS += -dynamic
stldbg-shared : CXXFLAGS += -dynamic
endif

ifeq ($(OSNAME),hp-ux)
ifneq ($(M_ARCH),ia64)
release-static : OPT += -fno-reorder-blocks
release-shared : OPT += -fno-reorder-blocks
endif
CCFLAGS = $(PTHREAD) $(OPT)
CFLAGS = $(PTHREAD) $(OPT)
# CXXFLAGS = $(PTHREAD) -nostdinc++ -fexceptions $(OPT)
CXXFLAGS = $(PTHREAD) -fexceptions $(OPT)
endif

ifeq ($(CXX_VERSION_MAJOR),2)
CXXFLAGS += -ftemplate-depth-32
endif

# Required for correct order of static objects dtors calls:
ifeq ("$(findstring $(OSNAME),darwin cygming)","")
ifneq ($(CXX_VERSION_MAJOR),2)
CXXFLAGS += $(_CXA_ATEXIT)
endif
endif

# Code should be ready for this option
ifneq ($(OSNAME),cygming)
ifneq ($(CXX_VERSION_MAJOR),2)
ifneq ($(CXX_VERSION_MAJOR),3)
CXXFLAGS += -fvisibility=hidden
CFLAGS += -fvisibility=hidden
endif
endif
endif

ifdef EXTRA_CXXFLAGS
CXXFLAGS += ${EXTRA_CXXFLAGS}
endif

ifdef EXTRA_CFLAGS
CFLAGS += ${EXTRA_CFLAGS}
endif

CDEPFLAGS = -E -M
CCDEPFLAGS = -E -M

# STLport DEBUG mode specific defines
stldbg-static :	    DEFS += -D_STLP_DEBUG
stldbg-shared :     DEFS += -D_STLP_DEBUG
stldbg-static-dep : DEFS += -D_STLP_DEBUG
stldbg-shared-dep : DEFS += -D_STLP_DEBUG

# optimization and debug compiler flags
release-static : OPT += -O2
release-shared : OPT += -O2

dbg-static : OPT += -g
dbg-shared : OPT += -g
#dbg-static-dep : OPT += -g
#dbg-shared-dep : OPT += -g

stldbg-static : OPT += -g
stldbg-shared : OPT += -g
#stldbg-static-dep : OPT += -g
#stldbg-shared-dep : OPT += -g

# dependency output parser (dependencies collector)

DP_OUTPUT_DIR = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_DBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_DBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_STLDBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_STLDBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

