# -*- makefile -*- Time-stamp: <07/05/31 01:29:22 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Copyright (c) 2006, 2007
# Francois Dumont
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

release-shared : LDFLAGS += -Tpd -w -w-dup
dbg-shared : LDFLAGS += -Tpd -w -w-dup
stldbg-shared : LDFLAGS += -Tpd -w -w-dup

ifneq ($(OSNAME),linux)

release-shared : LDFLAGS += -V4.0 -Gi
dbg-shared : LDFLAGS += -V4.0 -Gi
stldbg-shared : LDFLAGS += -V4.0 -Gi

release-shared : OPT += -tWD
dbg-shared : OPT += -tWD
stldbg-shared : OPT += -tWD

START_OBJ := c0d32.obj

else

release-shared : OPT += -tD -VP
dbg-shared : OPT += -tD -VP
stldbg-shared : OPT += -tD -VP

release-shared: DEFS += -D_DLL
dbg-shared:  DEFS += -D_DLL
stldbg-shared:  DEFS += -D_DLL

START_OBJ := borinitso.o

endif

# optimization and debug compiler flags

dbg-static : OPT += -R -v -y
dbg-shared : OPT += -R -v -y
stldbg-static : OPT += -R -v -y
stldbg-shared : OPT += -R -v -y

dbg-shared : LDFLAGS += -v
dbg-static : LDFLAGS += -v
stldbg-shared : LDFLAGS += -v
stldbg-static : LDFLAGS += -v

install-shared: install-release-shared install-dbg-shared install-stldbg-shared
install: install-shared

ifneq ($(OSNAME),linux)
install-dbg-shared: install-dbg-shared-tds 
install-stldbg-shared: install-stldbg-shared-tds
endif

BASE_LIBNAME := $(LIB_PREFIX)${LIBNAME}${LIB_TYPE}${LIB_SUFFIX}
BASE_LIBNAME_DBG := $(LIB_PREFIX)${LIBNAME}${DBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}
BASE_LIBNAME_STLDBG := $(LIB_PREFIX)${LIBNAME}${STLDBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}
BASE_LIBNAMES = ${BASE_LIBNAME} ${BASE_LIBNAME_DBG} ${BASE_LIBNAME_STLDBG}
BASE_LIB_EXTS = lib dll tds map res
LIB_FILES := $(foreach n,$(BASE_LIBNAMES),$(foreach e,$(BASE_LIB_EXTS),$(n).$(e)))

install-dbg-shared-tds:
	$(INSTALL_SO) $(OUTPUT_DIR_DBG)/${BASE_LIBNAME_DBG}.tds $(INSTALL_BIN_DIR)/

install-stldbg-shared-tds:
	$(INSTALL_SO) $(OUTPUT_DIR_STLDBG)/${BASE_LIBNAME_STLDBG}.tds $(INSTALL_BIN_DIR)/

clean::
	$(foreach d,$(OUTPUT_DIRS),$(foreach f,$(LIB_FILES),@rm -f $(d)/$(f)))
 
uninstall::
	$(foreach d,$(INSTALL_DIRS),$(foreach f,$(LIB_FILES),@rm -f $(d)/$(f)))
	$(foreach d,$(INSTALL_DIRS),@-rmdir -p $(d) 2>/dev/null)
