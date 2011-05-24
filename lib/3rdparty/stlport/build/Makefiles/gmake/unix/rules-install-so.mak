# -*- makefile -*- Time-stamp: <07/12/12 01:52:19 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifndef INSTALL_TAGS

ifndef _NO_SHARED_BUILD
INSTALL_TAGS := install-release-shared
else
INSTALL_TAGS := 
endif

ifdef _STATIC_BUILD
INSTALL_TAGS += install-release-static
endif

ifndef _NO_DBG_BUILD
ifndef _NO_SHARED_BUILD
INSTALL_TAGS += install-dbg-shared
endif
ifdef _STATIC_BUILD
INSTALL_TAGS += install-dbg-static
endif
endif

ifndef _NO_STLDBG_BUILD
ifndef WITHOUT_STLPORT
ifndef _NO_SHARED_BUILD
INSTALL_TAGS += install-stldbg-shared
endif
ifdef _STATIC_BUILD
INSTALL_TAGS += install-stldbg-static
endif
endif
endif

endif


ifndef INSTALL_STRIP_TAGS

ifndef _NO_SHARED_BUILD
INSTALL_STRIP_TAGS := install-strip-shared
else
INSTALL_STRIP_TAGS := 
endif

ifdef _STATIC_BUILD
INSTALL_STRIP_TAGS += install-release-static
endif

ifndef _NO_DBG_BUILD
ifndef _NO_SHARED_BUILD
INSTALL_STRIP_TAGS += install-dbg-shared
endif
ifdef _STATIC_BUILD
INSTALL_STRIP_TAGS += install-dbg-static
endif
endif

ifndef _NO_STLDBG_BUILD
ifndef WITHOUT_STLPORT
ifndef _NO_SHARED_BUILD
INSTALL_STRIP_TAGS += install-stldbg-shared
endif
ifdef _STATIC_BUILD
INSTALL_STRIP_TAGS += install-stldbg-static
endif
endif
endif

endif


PHONY += install install-strip install-headers $(INSTALL_TAGS) $(INSTALL_STRIP_TAGS)

install:	$(INSTALL_TAGS) install-headers

install-strip:	$(INSTALL_STRIP_TAGS) install-headers

# Workaround for GNU make 3.80; see comments in rules-so.mak
define do_install_so_links
$${INSTALL_LIB_DIR$(1)}/$${SO_NAME$(1)xxx}:	$${SO_NAME_OUT$(1)xxx}
	$$(INSTALL_SO) $${SO_NAME_OUT$(1)xxx} $$(INSTALL_LIB_DIR$(1))
	@$(call do_so_links_1,$$(INSTALL_LIB_DIR$(1)),$${SO_NAME$(1)xx},$${SO_NAME$(1)xxx})
	@$(call do_so_links_1,$$(INSTALL_LIB_DIR$(1)),$${SO_NAME$(1)x},$${SO_NAME$(1)xx})
	@$(call do_so_links_1,$$(INSTALL_LIB_DIR$(1)),$${SO_NAME$(1)},$${SO_NAME$(1)x})
endef

# Workaround for GNU make 3.80; see comments in rules-so.mak
define do_install_so_links_wk
# expand to nothing, if equal
ifneq (${INSTALL_LIB_DIR}/${SO_NAMExxx},${INSTALL_LIB_DIR_STLDBG}/${SO_NAME_STLDBGxxx})
# expand to nothing, if WITHOUT_STLPORT
ifndef WITHOUT_STLPORT
$(call do_install_so_links,$(1))
endif
endif
endef

# Workaround for GNU make 3.80; see comments in rules-so.mak
define do_install_so_links_wk2
# expand to nothing, if equal
ifneq (${INSTALL_LIB_DIR}/${SO_NAMExxx},${INSTALL_LIB_DIR_DBG}/${SO_NAME_DBGxxx})
$(call do_install_so_links,$(1))
endif
endef


$(eval $(call do_install_so_links,))
# ifneq (${INSTALL_LIB_DIR}/${SO_NAMExxx},${INSTALL_LIB_DIR_DBG}/${SO_NAME_DBGxxx})
# $(eval $(call do_install_so_links,_DBG))
$(eval $(call do_install_so_links_wk2,_DBG))
# endif
# ifneq (${INSTALL_LIB_DIR}/${SO_NAMExxx},${INSTALL_LIB_DIR_STLDBG}/${SO_NAME_STLDBGxxx})
# ifndef WITHOUT_STLPORT
$(eval $(call do_install_so_links_wk,_STLDBG))
# endif
# endif

install-release-shared:	release-shared $(INSTALL_LIB_DIR) $(INSTALL_LIB_DIR)/${SO_NAMExxx} install-headers
	${POST_INSTALL}

install-strip-shared:	release-shared $(INSTALL_LIB_DIR) $(INSTALL_LIB_DIR)/${SO_NAMExxx} install-headers
	${STRIP} ${_SO_STRIP_OPTION} $(INSTALL_LIB_DIR)/${SO_NAMExxx}
	${POST_INSTALL}

install-dbg-shared:	dbg-shared $(INSTALL_LIB_DIR_DBG) $(INSTALL_LIB_DIR_DBG)/${SO_NAME_DBGxxx}
	${POST_INSTALL_DBG}

ifndef WITHOUT_STLPORT
install-stldbg-shared:	stldbg-shared $(INSTALL_LIB_DIR_STLDBG) $(INSTALL_LIB_DIR_STLDBG)/${SO_NAME_STLDBGxxx}
	${POST_INSTALL_STLDBG}
endif

define do_install_headers
if [ ! -d $(INSTALL_HDR_DIR) ]; then \
  $(INSTALL_D) $(INSTALL_HDR_DIR) || { echo "Can't create $(INSTALL_HDR_DIR)"; exit 1; }; \
fi; \
for dd in $(HEADERS_BASE); do \
  d=`dirname $$dd`; \
  h=`basename $$dd`; \
  f=`cd $$d; find $$h \( -wholename "*/.svn" -prune \) -o \( -type d -print \)`; \
  for ddd in $$f; do \
    if [ ! -d $(INSTALL_HDR_DIR)/$$ddd ]; then \
      $(INSTALL_D) $(INSTALL_HDR_DIR)/$$ddd || { echo "Can't create $(INSTALL_HDR_DIR)/$$ddd"; exit 1; }; \
    fi; \
  done; \
  f=`find $$dd \( -wholename "*/.svn*" -o -name "*~" -o -name "*.bak" \) -prune -o \( -type f -print \)`; \
  for ff in $$f; do \
    h=`echo $$ff | sed -e "s|$$d|$(INSTALL_HDR_DIR)|"`; \
    $(INSTALL_F) $$ff $$h; \
  done; \
done; \
for f in $(HEADERS); do \
  h=`basename $$f`; \
  $(INSTALL_F) $$f $(INSTALL_HDR_DIR)/$$h || { echo "Can't install $(INSTALL_HDR_DIR)/$$h"; exit 1; }; \
done
endef

# find $$dd \( -type f \( \! \( -wholename "*/.svn*" -o -name "*~" -o -name "*.bak" \) \) \) -print
# _HEADERS_FROM = $(shell for dd in $(HEADERS_BASE); do find $$dd \( -type f \( \! \( -wholename "*/.svn/*" -o -name "*~" -o -name "*.bak" \) \) \) -print ; done )
# _HEADERS_TO = $(foreach d,$(HEADERS_BASE),$(patsubst $(d)/%,$(BASE_INSTALL_DIR)include/%,$(_HEADERS_FROM)))

install-headers:
	@$(do_install_headers)
