# -*- Makefile -*- Time-stamp: <07/05/31 22:18:20 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

PHONY += clean distclean mostlyclean maintainer-clean uninstall

define obj_clean
clean::
	@-rm -f $$($(1)_OBJ) $$($(1)_DEP)
	@-rm -f $$($(1)_OBJ_DBG) $$($(1)_DEP_DBG)
	@-rm -f $$($(1)_OBJ_STLDBG) $$($(1)_DEP_STLDBG)
endef

clean::	
	@-rm -f core core.*
ifdef PRGNAME
	@-rm -f $(OBJ) $(DEP)
	@-rm -f $(OBJ_DBG) $(DEP_DBG)
	@-rm -f $(OBJ_STLDBG) $(DEP_STLDBG)
endif
ifdef LIBNAME
	@-rm -f $(OBJ) $(DEP) $(_LSUPCPP_AUX_OBJ) $(_LSUPCPP_AUX_TSMP)
	@-rm -f $(OBJ_DBG) $(DEP_DBG)
	@-rm -f $(OBJ_STLDBG) $(DEP_STLDBG)
endif

$(foreach prg,$(PRGNAMES),$(eval $(call obj_clean,$(prg))))

$(foreach prg,$(LIBNAMES),$(eval $(call obj_clean,$(prg))))

distclean::	clean
# $(DEPENDS_COLLECTION) removed before directory,
# see app/clean.mak and lib/clean.mak

mostlyclean::	clean
	@-rm -f $(DEPENDS_COLLECTION)
	@-rm -f TAGS tags

maintainer-clean::	distclean
	@rm -f ${RULESBASE}/gmake/config.mak
	@-rm -f TAGS tags
