# -*- makefile -*- Time-stamp: <07/06/08 23:34:51 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

LDFLAGS ?= 

ifneq ("$(findstring $(OSNAME),darwin cygming)","")
include ${RULESBASE}/gmake/${OSNAME}/lib.mak
else
include ${RULESBASE}/gmake/unix/lib.mak
endif

include ${RULESBASE}/gmake/lib/${COMPILER_NAME}.mak

ifneq ("$(findstring $(OSNAME),cygming)","")
include ${RULESBASE}/gmake/${OSNAME}/rules-so.mak
else
include ${RULESBASE}/gmake/unix/rules-so.mak
endif

include ${RULESBASE}/gmake/lib/rules-a.mak

ifneq ("$(findstring $(OSNAME),cygming)","")
include ${RULESBASE}/gmake/${OSNAME}/rules-install-so.mak
else
include ${RULESBASE}/gmake/unix/rules-install-so.mak
endif

include ${RULESBASE}/gmake/lib/rules-install-a.mak
include ${RULESBASE}/gmake/lib/clean.mak
