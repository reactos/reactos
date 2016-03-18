# -*- makefile -*- Time-stamp: <07/05/31 01:29:36 ptr>
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

release-shared: OPT += -WD
dbg-shared: OPT += -WD
stldbg-shared: OPT += -WD

release-shared: LDFLAGS += /DELEXECUTABLE/IMPLIB:$(subst /,\\,$(OUTPUT_DIR)/$(SO_NAME_BASE).lib)
dbg-shared: LDFLAGS += /CODEVIEW/DELEXECUTABLE/IMPLIB:$(subst /,\\,$(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).lib)
stldbg-shared: LDFLAGS += /CODEVIEW/DELEXECUTABLE/IMPLIB:$(subst /,\\,$(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).lib)

DEF_OPTION = $(OUTPUT_DIR)/$(SO_NAME_BASE).def
DEF_OPTION_DBG = $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
DEF_OPTION_STLDBG = $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
