# -*- makefile -*- Time-stamp: <08/06/12 15:54:12 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifdef PRGNAME
PRG        := $(OUTPUT_DIR)/${PRGNAME}${EXE}
PRG_DBG    := $(OUTPUT_DIR_DBG)/${PRGNAME}${EXE}
PRG_STLDBG := $(OUTPUT_DIR_STLDBG)/${PRGNAME}${EXE}
endif

ALLPRGS = ${PRG}
ALLPRGS_DBG = ${PRG_DBG}
ALLPRGS_STLDBG = ${PRG_STLDBG}

define prog_prog
$(1)_PRG        := $(OUTPUT_DIR)/$(1)${EXE}
$(1)_PRG_DBG    := $(OUTPUT_DIR_DBG)/$(1)${EXE}
$(1)_PRG_STLDBG := $(OUTPUT_DIR_STLDBG)/$(1)${EXE}

ALLPRGS        += $${$(1)_PRG}
ALLPRGS_DBG    += $${$(1)_PRG_DBG}
ALLPRGS_STLDBG += $${$(1)_PRG_STLDBG}
endef

$(foreach prg,$(PRGNAMES),$(eval $(call prog_prog,$(prg))))

include ${RULESBASE}/gmake/app/${COMPILER_NAME}.mak
include ${RULESBASE}/gmake/app/rules.mak
include ${RULESBASE}/gmake/app/rules-install.mak

define prog_clean
clean::
	@rm -f $${$(1)_PRG} $${$(1)_PRG_DBG} $${$(1)_PRG_STLDBG}

uninstall::
	@rm -f $$(INSTALL_BIN_DIR)/$$($(1)_PRG) $$(INSTALL_BIN_DIR_DBG)/$$($(1)_PRG_DBG) $$(INSTALL_BIN_DIR_STLDBG)/$$($(1)_PRG_STLDBG)
endef

clean::
ifdef PRGNAME
	@-rm -f ${PRG} ${PRG_DBG} ${PRG_STLDBG}
endif

$(foreach prg,$(PRGNAMES),$(eval $(call prog_clean,$(prg))))

distclean::
	@-rm -f $(DEPENDS_COLLECTION)
	@-rmdir -p ${OUTPUT_DIR} ${OUTPUT_DIR_DBG} ${OUTPUT_DIR_STLDBG} 2>/dev/null

uninstall::
ifdef PRGNAME
	@-rm -f $(INSTALL_BIN_DIR)/${PRGNAME}${EXE} $(INSTALL_BIN_DIR_DBG)/${PRGNAME}${EXE} $(INSTALL_BIN_DIR_STLDBG)/${PRGNAME}${EXE}
endif
	@-rmdir -p $(INSTALL_BIN_DIR) $(INSTALL_BIN_DIR_DBG) $(INSTALL_BIN_DIR_STLDBG) 2>/dev/null
