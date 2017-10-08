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

checks:	
	file ${PRG} | grep ELF >/dev/null || exit 1
	@echo
	@echo release ok
	@echo

checks-dbg:
	file ${PRG_DBG} | grep ELF >/dev/null || exit 1
	@echo
	@echo dbg ok
	@echo

ifndef WITHOUT_STLPORT
checks-stldbg:
	file ${PRG_STLDBG} | grep ELF >/dev/null || exit 1
	@echo
	@echo stldbg ok
	@echo
endif

PHONY += checks checks-dbg checks-stldbg
