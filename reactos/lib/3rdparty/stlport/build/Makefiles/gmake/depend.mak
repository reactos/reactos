# Time-stamp: <07/02/05 12:57:11 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

PHONY += release-static-dep release-shared-dep dbg-static-dep dbg-shared-dep \
         depend

ifndef WITHOUT_STLPORT
PHONY += stldbg-static-dep stldbg-shared-dep
endif

release-static-dep release-shared-dep:	$(DEP)

dbg-static-dep dbg-shared-dep:	$(DEP_DBG)

ifndef WITHOUT_STLPORT
stldbg-static-dep stldbg-shared-dep:	$(DEP_STLDBG)

_ALL_DEP := $(DEP) $(DEP_DBG) $(DEP_STLDBG)
_DASH_DEP := release-shared-dep dbg-shared-dep stldbg-shared-dep
else
_ALL_DEP := $(DEP) $(DEP_DBG)
_DASH_DEP := release-shared-dep dbg-shared-dep
endif


depend::	$(OUTPUT_DIRS) ${_DASH_DEP}
	@cat -s $(_ALL_DEP) /dev/null > $(DEPENDS_COLLECTION)

TAGS:	$(OUTPUT_DIRS) ${_DASH_DEP}
	@cat -s $(_ALL_DEP) /dev/null | sed -e 's/^.*://;s/^ *//;s/\\$$//;s/ $$//;s/ /\n/g' | sort | uniq | xargs etags -I --declarations 

ifneq ($(OSREALNAME),mingw)
tags:	$(OUTPUT_DIRS) ${_DASH_DEP}
	@cat -s $(_ALL_DEP) /dev/null | sed -e 's/^.*://;s/^ *//;s/\\$$//;s/ $$//;s/ /\n/g' | sort | uniq | xargs ctags -d --globals --declarations -t -T
endif

-include $(DEPENDS_COLLECTION)
