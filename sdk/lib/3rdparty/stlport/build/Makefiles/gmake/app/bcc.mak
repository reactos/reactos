# -*- Makefile -*- Time-stamp: <07/05/31 01:05:40 ptr>
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

ifneq ($(OSNAME),linux)

OPT += -tWC -w-par

LDFLAGS += -Tpe -w -w-dup

START_OBJ = c0x32.obj

LDFLAGS += -L$(subst /,\\,$(STLPORT_DIR)/lib)

ifdef WITH_DYNAMIC_RTL
release-static: DEFS += -D_STLP_USE_STATIC_LIB
dbg-static:  DEFS += -D_STLP_USE_STATIC_LIB
stldbg-static:  DEFS += -D_STLP_USE_STATIC_LIB
endif

ifdef WITH_STATIC_RTL
release-shared: DEFS += -D_STLP_USE_DYNAMIC_LIB
dbg-shared:  DEFS += -D_STLP_USE_DYNAMIC_LIB
stldbg-shared:  DEFS += -D_STLP_USE_DYNAMIC_LIB
endif

else

OPT += -tC

LDFLAGS += -ap 
 
START_OBJ = borinit.o crt1.o

endif

ifdef USE_BCC_DBG_OPTS

# optimization and debug compiler flags

dbg-static : OPT += -R -v -y
dbg-shared : OPT += -R -v -y
stldbg-static : OPT += -R -v -y
stldbg-shared : OPT += -R -v -y

dbg-shared : LDFLAGS += -v
dbg-static : LDFLAGS += -v
stldbg-shared : LDFLAGS += -v
stldbg-static : LDFLAGS += -v

install-dbg-shared: install-dbg-shared-tds 
install-stldbg-shared: install-stldbg-shared-tds

install-dbg-static: install-dbg-static-tds 
install-stldbg-static: install-stldbg-static-tds

install-dbg-shared-tds:
	$(INSTALL_EXE) $(OUTPUT_DIR_DBG)/${PRGNAME}.tds $(INSTALL_BIN_DIR_DBG)/${PRGNAME}.tds

install-stldbg-shared-tds:
	$(INSTALL_EXE) $(OUTPUT_DIR_STLDBG)/${PRGNAME}.tds $(INSTALL_BIN_DIR_STLDBG)/${PRGNAME}.tds

install-dbg-static-tds:
	$(INSTALL_EXE) $(OUTPUT_DIR_DBG)/${PRGNAME}.tds $(INSTALL_BIN_DIR_DBG)/${PRGNAME}.tds

install-stldbg-static-tds:
	$(INSTALL_EXE) $(OUTPUT_DIR_STLDBG)/${PRGNAME}.tds $(INSTALL_BIN_DIR_STLDBG)/${PRGNAME}.tds

else

dbg-shared : OPT += -vi-
dbg-static : OPT += -vi-
stldbg-shared : OPT += -vi-
stldbg-static : OPT += -vi-

endif

PRG_FILES := ${PRGNAME}${EXE} ${PRGNAME}.tds ${PRGNAME}.map
TMP_FILES := test.txt test_file.txt win32_file_format.tmp

clean::
	$(foreach d, $(OUTPUT_DIRS), $(foreach f, $(PRG_FILES), @rm -f $(d)/$(f)))
 
uninstall::
	$(foreach d, $(INSTALL_DIRS), $(foreach f, $(PRG_FILES), @rm -f $(d)/$(f)))
	$(foreach d, $(INSTALL_DIRS), $(foreach f, $(TMP_FILES), @rm -f $(d)/$(f)))
	$(foreach d, $(INSTALL_DIRS), @-rmdir -p $(d) 2>/dev/null)

