# Time-stamp: <08/01/12 00:50:38 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

INCLUDES :=

ifndef _FORCE_CXX
CXX := CC
else
CXX := ${_FORCE_CXX}
endif

ifndef _FORCE_CC
CC := cc
else
CC := ${_FORCE_CC}
endif

CXX_VERSION := $(shell ${CXX} -V 2>&1 | grep ${CXX} | awk '{ print $$4; }')

CXX_VERSION_MAJOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$1; }')
CXX_VERSION_MINOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$2; }')
CXX_VERSION_PATCH := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { if (NF > 2) {print $$3;}else{print "0"} }')

DEFS ?=
OPT ?=

OUTPUT_OPTION = -o $@
LINK_OUTPUT_OPTION = ${OUTPUT_OPTION}
CPPFLAGS = $(DEFS) $(INCLUDES)

OPT += -mt +w2
CCFLAGS = -erroff=doubunder -qoption ccfe -expand=1000 -library=no%Cstd,no%iostream,no%rwtools7-xildoff $(OPT) 
CFLAGS = $(OPT)
CXXFLAGS = -erroff=doubunder -qoption ccfe -expand=1000 -library=no%Cstd,no%iostream,no%rwtools7 -xildoff $(OPT)
CDEPFLAGS = -xM
CCDEPFLAGS = -xM

# STLport DEBUG mode specific defines
stldbg-static :	    DEFS += -D_STLP_DEBUG
stldbg-shared :     DEFS += -D_STLP_DEBUG
stldbg-static-dep : DEFS += -D_STLP_DEBUG
stldbg-shared-dep : DEFS += -D_STLP_DEBUG

# optimization and debug compiler flags
release-static : OPT += -xO2
release-shared : OPT += -xO2

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


