# -*- makefile -*- Time-stamp: <06/12/12 09:37:04 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifndef WITHOUT_STLPORT
install:	install-release-shared install-dbg-shared install-stldbg-shared
else
install:	install-release-shared install-dbg-shared
endif

INSTALL_PRGNAME_CMD =
INSTALL_PRGNAME_CMD_DBG =
INSTALL_PRGNAME_CMD_STLDBG =

define prog_install
INSTALL_$(1)_PRGNAME := $(1)${EXE}
INSTALL_PRGNAME_CMD += $$(INSTALL_EXE) $${$(1)_PRG} $$(INSTALL_BIN_DIR)/$${INSTALL_$(1)_PRGNAME}; \

INSTALL_$(1)_PRGNAME_DBG := $${INSTALL_$(1)_PRGNAME}
INSTALL_PRGNAME_CMD_DBG += $$(INSTALL_EXE) $${$(1)_PRG_DBG} $$(INSTALL_BIN_DIR_DBG)/$${INSTALL_$(1)_PRGNAME_DBG}; \

ifndef WITHOUT_STLPORT
INSTALL_$(1)_PRGNAME_STLDBG := $${INSTALL_$(1)_PRGNAME}
INSTALL_PRGNAME_CMD_STLDBG += $$(INSTALL_EXE) $${$(1)_PRG_STLDBG} $$(INSTALL_BIN_DIR_STLDBG)/$${INSTALL_$(1)_PRGNAME_STLDBG}; \

endif
endef

INSTALL_PRGNAME := ${PRGNAME}${EXE}
$(foreach prg,$(PRGNAMES),$(eval $(call prog_install,$(prg))))

INSTALL_PRGNAME_DBG := ${INSTALL_PRGNAME}

ifndef WITHOUT_STLPORT
INSTALL_PRGNAME_STLDBG := ${INSTALL_PRGNAME}
endif

install-release-shared: release-shared $(INSTALL_BIN_DIR)
ifdef PRGNAME
	$(INSTALL_EXE) ${PRG} $(INSTALL_BIN_DIR)/${INSTALL_PRGNAME}
endif
	$(INSTALL_PRGNAME_CMD)
	$(POST_INSTALL)

install-dbg-shared: dbg-shared $(INSTALL_BIN_DIR_DBG)
ifdef PRGNAME
	$(INSTALL_EXE) ${PRG_DBG} $(INSTALL_BIN_DIR_DBG)/${INSTALL_PRGNAME_DBG}
endif
	$(INSTALL_PRGNAME_CMD_DBG)
	$(POST_INSTALL_DBG)

ifndef WITHOUT_STLPORT
install-stldbg-shared: stldbg-shared $(INSTALL_BIN_DIR_STLDBG)
ifdef PRGNAME
	$(INSTALL_EXE) ${PRG_STLDBG} $(INSTALL_BIN_DIR_STLDBG)/${INSTALL_PRGNAME_STLDBG}
endif
	$(INSTALL_PRGNAME_CMD_STLDBG)
	$(POST_INSTALL_STLDBG)
endif
