# -*- Makefile -*- Time-stamp: <06/11/13 23:03:45 ptr>

SRCROOT := ../../..

EXTRA_POST := checks
EXTRA_POST_DBG := checks-dbg
EXTRA_POST_STLDBG := checks-stldbg

include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

ifndef WITHOUT_STLPORT
LDFLAGS += -Wl,-rpath=${STLPORT_LIB_DIR}
endif

# Check that we really use C compiler here

define ldd_check
r=`ldd $(1) | grep $(2)`; if [ "$$r" != "" ]; then exit 1; fi
endef


checks:	
	file ${PRG} | grep ELF >/dev/null || exit 1
	${call ldd_check,${PRG},stlport}
	${call ldd_check,${PRG},stdc++}
	${call ldd_check,${PRG},gcc_s}
	@echo
	@echo release ok
	@echo

checks-dbg:
	file ${PRG_DBG} | grep ELF >/dev/null || exit 1
	${call ldd_check,${PRG_DBG},stlport}
	${call ldd_check,${PRG_DBG},stdc++}
	${call ldd_check,${PRG_DBG},gcc_s}
	@echo
	@echo dbg ok
	@echo

ifndef WITHOUT_STLPORT
checks-stldbg:
	file ${PRG_STLDBG} | grep ELF >/dev/null || exit 1
	${call ldd_check,${PRG_STLDBG},stlport}
	${call ldd_check,${PRG_STLDBG},stdc++}
	${call ldd_check,${PRG_STLDBG},gcc_s}
	@echo
	@echo stldbg ok
	@echo
endif

PHONY += checks checks-dbg checks-stldbg
