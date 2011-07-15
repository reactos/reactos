# -*- Makefile -*- Time-stamp: <07/05/31 01:05:57 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Copyright (c) 2006, 2007
# Francois Dumont
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

CXXFLAGS += -w6 -w7 -w18

stldbg-shared : CXXFLAGS += -HP50
stldbg-static : CXXFLAGS += -HP50

OPT += -WA

release-shared : LDFLAGS += /DELEXECUTABLE
release-static : LDFLAGS += /DELEXECUTABLE
dbg-shared : LDFLAGS += /DELEXECUTABLE/CODEVIEW/NOCVPACK
dbg-static : LDFLAGS += /DELEXECUTABLE/CODEVIEW/NOCVPACK
stldbg-shared : LDFLAGS += /DELEXECUTABLE/CODEVIEW/NOCVPACK
stldbg-static : LDFLAGS += /DELEXECUTABLE/CODEVIEW/NOCVPACK

# workaround for stl/config/_auto_link.h
STL_LIBNAME = stlport
DBG_SUFFIX := d
STLDBG_SUFFIX := stld

ifdef LIB_MOTIF
LIB_SUFFIX := _$(LIB_MOTIF).${LIBMAJOR}.${LIBMINOR}
else
LIB_SUFFIX := .${LIBMAJOR}.${LIBMINOR}
endif

# Shared libraries:
ifdef WITH_STATIC_RTL
LIB_TYPE := _x
else
LIB_TYPE := 
endif

LIB_NAME := $(LIB_PREFIX)${STL_LIBNAME}${LIB_TYPE}${LIB_SUFFIX}.$(LIB)
LIB_NAME_DBG := $(LIB_PREFIX)${STL_LIBNAME}${DBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}.$(LIB)
LIB_NAME_STLDBG := $(LIB_PREFIX)${STL_LIBNAME}${STLDBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}.$(LIB)

# Static libraries:
ifdef WITH_DYNAMIC_RTL
A_LIB_TYPE := _statix
else
A_LIB_TYPE := _static
endif

A_NAME := $(LIB_PREFIX)${STL_LIBNAME}${A_LIB_TYPE}${LIB_SUFFIX}.$(ARCH)
A_NAME_DBG := $(LIB_PREFIX)${STL_LIBNAME}${DBG_SUFFIX}${A_LIB_TYPE}${LIB_SUFFIX}.${ARCH}
A_NAME_STLDBG := ${LIB_PREFIX}${STL_LIBNAME}${STLDBG_SUFFIX}${A_LIB_TYPE}${LIB_SUFFIX}.${ARCH}

release-shared : LDLIBS += $(STLPORT_DIR)/lib/$(LIB_NAME)
dbg-shared : LDLIBS += $(STLPORT_DIR)/lib/$(LIB_NAME_DBG)
stldbg-shared : LDLIBS += $(STLPORT_DIR)/lib/$(LIB_NAME_STLDBG)
release-static : LDLIBS += $(STLPORT_DIR)/lib/$(A_NAME)
dbg-static : LDLIBS += $(STLPORT_DIR)/lib/$(A_NAME_DBG)
stldbg-static : LDLIBS += $(STLPORT_DIR)/lib/$(A_NAME_STLDBG)
