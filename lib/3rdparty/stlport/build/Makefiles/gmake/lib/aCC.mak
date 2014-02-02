# -*- makefile -*- Time-stamp: <08/06/12 14:59:23 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

dbg-shared:	LDFLAGS += -b +nostl -Wl,+h$(SO_NAME_DBGxx)
stldbg-shared:	LDFLAGS += -b +nostl -Wl,+h$(SO_NAME_STLDBGxx)
release-shared:	LDFLAGS += -b +nostl -Wl,+h$(SO_NAMExx)
