# Time-stamp: <07/05/30 23:59:42 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

PRGS_DIR_SRC =
define prog_
PRGS_DIR_SRC += $$(dir $${$(1)_SRC_CPP} $${$(1)_SRC_CC} $${$(1)_SRC_CXX} $${$(1)_SRC_C} $${$(1)_SRC_S} )
$(1)_ALLBASE := $$(basename $$(notdir $${$(1)_SRC_CC} $${$(1)_SRC_CPP} $${$(1)_SRC_CXX} $${$(1)_SRC_C} $${$(1)_SRC_S} ) )
$(1)_ALLOBJS    := $$(addsuffix .o,$${$(1)_ALLBASE})
$(1)_ALLDEPS    := $$(addsuffix .d,$${$(1)_ALLBASE})

$(1)_OBJ        := $$(addprefix $$(OUTPUT_DIR)/,$${$(1)_ALLOBJS})
$(1)_OBJ_DBG    := $$(addprefix $$(OUTPUT_DIR_DBG)/,$${$(1)_ALLOBJS})
$(1)_OBJ_STLDBG := $$(addprefix $$(OUTPUT_DIR_STLDBG)/,$${$(1)_ALLOBJS})

$(1)_DEP        := $$(addprefix $$(OUTPUT_DIR)/,$${$(1)_ALLDEPS})
$(1)_DEP_DBG    := $$(addprefix $$(OUTPUT_DIR_DBG)/,$${$(1)_ALLDEPS})
$(1)_DEP_STLDBG := $$(addprefix $$(OUTPUT_DIR_STLDBG)/,$${$(1)_ALLDEPS})

$(1)_RES        := $$(addprefix $$(OUTPUT_DIR)/,$${$(1)_ALLRESS})
$(1)_RES_DBG    := $$(addprefix $$(OUTPUT_DIR_DBG)/,$${$(1)_ALLRESS})
$(1)_RES_STLDBG := $$(addprefix $$(OUTPUT_DIR_STLDBG)/,$${$(1)_ALLRESS})

ifeq ("$$(sort $${$(1)_SRC_CC} $${$(1)_SRC_CPP} $${$(1)_SRC_CXX})","")
$(1)_NOT_USE_NOSTDLIB := 1
_$(1)_C_SOURCES_ONLY := true
endif

endef

$(foreach prg,$(PRGNAMES),$(eval $(call prog_,$(prg))))

# If we have no C++ sources, let's use C compiler for linkage instead of C++.
ifeq ("$(sort ${SRC_CC} ${SRC_CPP} ${SRC_CXX})","")
NOT_USE_NOSTDLIB := 1
_C_SOURCES_ONLY := true
endif

# if sources disposed in several dirs, calculate appropriate rules

DIRS_UNIQUE_SRC := $(dir $(SRC_CPP) $(SRC_CC) $(SRC_CXX) $(SRC_C) $(SRC_S) )
ifeq (${OSNAME},cygming)
DIRS_UNIQUE_SRC := ${DIRS_UNIQUE_SRC} $(dir $(SRC_RC) )
endif
DIRS_UNIQUE_SRC := $(sort $(DIRS_UNIQUE_SRC) $(PRGS_DIR_SRC))

# The rules below may be even simpler (i.e. define macro that generate
# rules for COMPILE.xx), but this GNU make 3.80 unhappy with it;
# GNU make 3.81 work fine, but 3.81 is new...
# The code below verbose, but this is price for compatibility with 3.80

define rule_o
$$(OUTPUT_DIR$(1))/%.o:	$(2)%.cc
	$$(COMPILE.cc) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.d:	$(2)%.cc
	@$$(COMPILE.cc) $$(CCDEPFLAGS) $$< $$(DP_OUTPUT_DIR$(1))

$$(OUTPUT_DIR$(1))/%.o:	$(2)%.cpp
	$$(COMPILE.cc) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.d:	$(2)%.cpp
	@$$(COMPILE.cc) $$(CCDEPFLAGS) $$< $$(DP_OUTPUT_DIR$(1))

$$(OUTPUT_DIR$(1))/%.o:	$(2)%.cxx
	$$(COMPILE.cc) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.d:	$(2)%.cxx
	@$$(COMPILE.cc) $$(CCDEPFLAGS) $$< $$(DP_OUTPUT_DIR$(1))

$$(OUTPUT_DIR$(1))/%.o:	$(2)%.c
	$$(COMPILE.c) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.d:	$(2)%.c
	@$$(COMPILE.c) $$(CCDEPFLAGS) $$< $$(DP_OUTPUT_DIR$(1))

$$(OUTPUT_DIR$(1))/%.o:	$(2)%.s
	$$(COMPILE.s) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.o:	$(2)%.S
	$$(COMPILE.S) $$(OUTPUT_OPTION) $$<

$$(OUTPUT_DIR$(1))/%.d:	$(2)%.S
	@$$(COMPILE.S) $$(SDEPFLAGS) $$< $$(DP_OUTPUT_DIR$(1))
endef

define rule_rc
$$(OUTPUT_DIR$(1))/%.res:	$(2)%.rc
	$$(COMPILE.rc) $$(RC_OUTPUT_OPTION) $$<
endef

define rules_
$(call rule_o,,$(1))
ifneq ($(OUTPUT_DIR),$(OUTPUT_DIR_A))
$(call rule_o,_A,$(1))
endif
$(call rule_o,_DBG,$(1))
ifneq ($(OUTPUT_DIR_DBG),$(OUTPUT_DIR_A_DBG))
$(call rule_o,_A_DBG,$(1))
endif
ifndef WITHOUT_STLPORT
$(call rule_o,_STLDBG,$(1))
ifneq ($(OUTPUT_DIR_STLDBG),$(OUTPUT_DIR_A_STLDBG))
$(call rule_o,_A_STLDBG,$(1))
endif
endif
ifeq ($(OSNAME),cygming)
$(call rule_rc,,$(1))
$(call rule_rc,_DBG,$(1))
ifndef WITHOUT_STLPORT
$(call rule_rc,_STLDBG,$(1))
endif
endif
endef

$(foreach dir,$(DIRS_UNIQUE_SRC),$(eval $(call rules_,$(dir))))

ALLBASE    := $(basename $(notdir $(SRC_CC) $(SRC_CPP) $(SRC_CXX) $(SRC_C) $(SRC_S)))
ifeq (${OSNAME},cygming)
RCBASE    += $(basename $(notdir $(SRC_RC)))
endif

ALLOBJS    := $(addsuffix .o,$(ALLBASE))
ALLDEPS    := $(addsuffix .d,$(ALLBASE))
ALLRESS    := $(addsuffix .res,$(RCBASE))

OBJ        := $(addprefix $(OUTPUT_DIR)/,$(ALLOBJS))
OBJ_DBG    := $(addprefix $(OUTPUT_DIR_DBG)/,$(ALLOBJS))
OBJ_STLDBG := $(addprefix $(OUTPUT_DIR_STLDBG)/,$(ALLOBJS))

DEP        := $(addprefix $(OUTPUT_DIR)/,$(ALLDEPS))
DEP_DBG    := $(addprefix $(OUTPUT_DIR_DBG)/,$(ALLDEPS))
DEP_STLDBG := $(addprefix $(OUTPUT_DIR_STLDBG)/,$(ALLDEPS))

RES        := $(addprefix $(OUTPUT_DIR)/,$(ALLRESS))
RES_DBG    := $(addprefix $(OUTPUT_DIR_DBG)/,$(ALLRESS))
RES_STLDBG := $(addprefix $(OUTPUT_DIR_STLDBG)/,$(ALLRESS))

ifeq ($(OUTPUT_DIR),$(OUTPUT_DIR_A))
OBJ_A      := $(OBJ)
DEP_A      := $(DEP)
else
OBJ_A      := $(addprefix $(OUTPUT_DIR_A)/,$(ALLOBJS))
DEP_A      := $(addprefix $(OUTPUT_DIR_A)/,$(ALLDEPS))
endif

ifeq ($(OUTPUT_DIR_DBG),$(OUTPUT_DIR_A_DBG))
OBJ_A_DBG  := $(OBJ_DBG)
DEP_A_DBG  := $(DEP_DBG)
else
OBJ_A_DBG  := $(addprefix $(OUTPUT_DIR_A_DBG)/,$(ALLOBJS))
DEP_A_DBG  := $(addprefix $(OUTPUT_DIR_A_DBG)/,$(ALLDEPS))
endif

ifeq ($(OUTPUT_DIR_STLDBG),$(OUTPUT_DIR_A_STLDBG))
OBJ_A_STLDBG := $(OBJ_STLDBG)
DEP_A_STLDBG := $(DEP_STLDBG)
else
OBJ_A_STLDBG := $(addprefix $(OUTPUT_DIR_A_STLDBG)/,$(ALLOBJS))
DEP_A_STLDBG := $(addprefix $(OUTPUT_DIR_A_STLDBG)/,$(ALLDEPS))
endif

