# -*- makefile -*- Time-stamp: <06/11/02 10:34:43 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

PHONY += install-release-static install-dbg-static install-stldbg-static

install-release-static:	release-static
	@if [ ! -d $(INSTALL_LIB_DIR) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT} $(INSTALL_LIB_DIR)

install-dbg-static:	dbg-static
	@if [ ! -d $(INSTALL_LIB_DIR_DBG) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR_DBG) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT_DBG} $(INSTALL_LIB_DIR_DBG)

ifndef WITHOUT_STLPORT

install-stldbg-static:	stldbg-static
	@if [ ! -d $(INSTALL_LIB_DIR_STLDBG) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR_STLDBG) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT_STLDBG} $(INSTALL_LIB_DIR_STLDBG)

endif
