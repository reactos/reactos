# -*- makefile -*- Time-stamp: <06/12/12 09:43:02 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

# Shared libraries tags

PHONY += release-shared dbg-shared stldbg-shared

release-shared:	$(EXTRA_PRE) $(OUTPUT_DIR) ${SO_NAME_OUTxxx} $(EXTRA_POST)

dbg-shared:	$(EXTRA_PRE_DBG) $(OUTPUT_DIR_DBG) ${SO_NAME_OUT_DBGxxx} $(EXTRA_POST_DBG)

ifndef WITHOUT_STLPORT
stldbg-shared:	$(EXTRA_PRE_STLDBG) $(OUTPUT_DIR_STLDBG) ${SO_NAME_OUT_STLDBGxxx} $(EXTRA_POST_STLDBG)
endif

define do_so_links_1
if [ -h $(1)/$(2) ]; then \
  if [ `readlink $(1)/$(2)` != "$(3)" ]; then \
    rm $(1)/$(2); \
    ln -s $(3) $(1)/$(2); \
  fi; \
else \
  ln -s $(3) $(1)/$(2); \
fi;
endef

# Workaround for GNU make 3.80: it fail on 'eval' within 'if'
# directive after some level of complexity, i.e. after complex
# rules it fails on code:
#
# $(eval $(call do_so_links,cc,))
# $(eval $(call do_so_links,cc,_DBG))
# ifndef WITHOUT_STLPORT
# $(eval $(call do_so_links,cc,_STLDBG))
# endif
#
# Put 'if' logic into defined macro looks as workaround.
#
# The GNU make 3.81 free from this problem, but it new...

define do_so_links
$${SO_NAME_OUT$(1)xxx}:	$$(OBJ$(1)) $$(LIBSDEP)
ifeq ("${_C_SOURCES_ONLY}","")
ifneq ($(COMPILER_NAME),bcc)
	$$(LINK.cc) $$(LINK_OUTPUT_OPTION) $${START_OBJ} $$(OBJ$(1)) $$(LDLIBS) $${STDLIBS} $${END_OBJ}
else
	$$(LINK.cc) $${START_OBJ} $$(OBJ$(1)) $${END_OBJ}, $$(LINK_OUTPUT_OPTION), , $$(LDLIBS) $${STDLIBS} 
endif
else
	$$(LINK.c) $$(LINK_OUTPUT_OPTION) $$(OBJ$(1)) $$(LDLIBS)
endif
	@$(call do_so_links_1,$$(OUTPUT_DIR$(1)),$${SO_NAME$(1)xx},$${SO_NAME$(1)xxx})
	@$(call do_so_links_1,$$(OUTPUT_DIR$(1)),$${SO_NAME$(1)x},$${SO_NAME$(1)xx})
	@$(call do_so_links_1,$$(OUTPUT_DIR$(1)),$${SO_NAME$(1)},$${SO_NAME$(1)x})
endef

define do_so_links_wk
# expand to nothing, if WITHOUT_STLPORT
ifndef WITHOUT_STLPORT
$(call do_so_links,$(1))
endif
endef

$(eval $(call do_so_links,))
$(eval $(call do_so_links,_DBG))
# ifndef WITHOUT_STLPORT
$(eval $(call do_so_links_wk,_STLDBG))
# endif
