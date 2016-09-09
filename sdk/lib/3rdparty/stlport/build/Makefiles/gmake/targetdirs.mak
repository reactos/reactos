# Time-stamp: <08/02/28 10:30:06 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2008
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifdef TARGET_OS
TARGET_NAME := ${TARGET_OS}-
else
TARGET_NAME :=
endif

BASE_OUTPUT_DIR        := obj
PRE_OUTPUT_DIR         := $(BASE_OUTPUT_DIR)/$(TARGET_NAME)$(COMPILER_NAME)
OUTPUT_DIR             := $(PRE_OUTPUT_DIR)/so$(EXTRA_DIRS)
OUTPUT_DIR_DBG         := $(PRE_OUTPUT_DIR)/so_g$(EXTRA_DIRS)
ifndef WITHOUT_STLPORT
OUTPUT_DIR_STLDBG      := $(PRE_OUTPUT_DIR)/so_stlg$(EXTRA_DIRS)
endif

# file to store generated dependencies for make:
DEPENDS_COLLECTION     := $(PRE_OUTPUT_DIR)/.make.depend

# catalog for auxilary files, if any
AUX_DIR                := $(PRE_OUTPUT_DIR)/.auxdir

# I use the same catalog, as for shared:
OUTPUT_DIR_A           := $(OUTPUT_DIR)
OUTPUT_DIR_A_DBG       := $(OUTPUT_DIR_DBG)
ifndef WITHOUT_STLPORT
OUTPUT_DIR_A_STLDBG    := $(OUTPUT_DIR_STLDBG)
endif

BASE_INSTALL_DIR       ?= /usr/local

BASE_INSTALL_LIB_DIR   ?= $(DESTDIR)${BASE_INSTALL_DIR}
BASE_INSTALL_BIN_DIR   ?= $(DESTDIR)${BASE_INSTALL_DIR}
BASE_INSTALL_HDR_DIR   ?= $(DESTDIR)${BASE_INSTALL_DIR}

INSTALL_LIB_DIR        ?= ${BASE_INSTALL_LIB_DIR}/${TARGET_NAME}lib
INSTALL_LIB_DIR_DBG    ?= ${BASE_INSTALL_LIB_DIR}/${TARGET_NAME}lib
ifndef WITHOUT_STLPORT
INSTALL_LIB_DIR_STLDBG ?= ${BASE_INSTALL_LIB_DIR}/${TARGET_NAME}lib
endif
INSTALL_BIN_DIR        ?= ${BASE_INSTALL_BIN_DIR}/${TARGET_NAME}bin
INSTALL_BIN_DIR_DBG    ?= ${INSTALL_BIN_DIR}_g
ifndef WITHOUT_STLPORT
INSTALL_BIN_DIR_STLDBG ?= ${INSTALL_BIN_DIR}_stlg
endif
INSTALL_HDR_DIR        ?= ${BASE_INSTALL_DIR}/include

ifndef WITHOUT_STLPORT
OUTPUT_DIRS := $(OUTPUT_DIR) $(OUTPUT_DIR_DBG) $(OUTPUT_DIR_STLDBG) \
               $(OUTPUT_DIR_A) $(OUTPUT_DIR_A_DBG) $(OUTPUT_DIR_A_STLDBG)
else
OUTPUT_DIRS := $(OUTPUT_DIR) $(OUTPUT_DIR_DBG) \
               $(OUTPUT_DIR_A) $(OUTPUT_DIR_A_DBG)
endif

ifndef WITHOUT_STLPORT
INSTALL_LIB_DIRS := $(INSTALL_LIB_DIR) $(INSTALL_LIB_DIR_DBG) $(INSTALL_LIB_DIR_STLDBG)
INSTALL_BIN_DIRS := $(INSTALL_BIN_DIR) $(INSTALL_BIN_DIR_DBG) $(INSTALL_BIN_DIR_STLDBG)
else
INSTALL_LIB_DIRS := $(INSTALL_LIB_DIR) $(INSTALL_LIB_DIR_DBG)
INSTALL_BIN_DIRS := $(INSTALL_BIN_DIR) $(INSTALL_BIN_DIR_DBG)
endif

# sort will remove duplicates:
OUTPUT_DIRS := $(sort $(OUTPUT_DIRS))
INSTALL_LIB_DIRS := $(sort $(INSTALL_LIB_DIRS))
INSTALL_BIN_DIRS := $(sort $(INSTALL_BIN_DIRS))
INSTALL_DIRS := $(sort $(INSTALL_LIB_DIRS) $(INSTALL_BIN_DIRS))

PHONY += $(OUTPUT_DIRS) $(INSTALL_DIRS) $(AUX_DIR)

define createdirs
@for d in $@ ; do \
  if [ -e $$d -a -f $$d ] ; then \
    echo "ERROR: Regular file $$d present, directory instead expected" ; \
    exit 1; \
  elif [ ! -d $$d ] ; then \
    mkdir -p $$d ; \
  fi ; \
done
endef

$(OUTPUT_DIRS):
	$(createdirs)

$(INSTALL_DIRS):
	$(createdirs)

$(AUX_DIR):
	$(createdirs)
