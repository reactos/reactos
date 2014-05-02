# -*- makefile -*- Time-stamp: <08/06/12 15:00:07 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

OPT += -KPIC

ifeq ($(OSNAME),linux)
dbg-shared:	LDFLAGS += -shared -Wl,-h$(SO_NAME_DBGxx)
stldbg-shared:	LDFLAGS += -shared -Wl,-h$(SO_NAME_STLDBGxx)
release-shared:	LDFLAGS += -shared -Wl,-h$(SO_NAMExx)
endif

