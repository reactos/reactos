# -*- makefile -*- Time-stamp: <07/05/31 22:11:48 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

# Static libraries tags

PHONY += release-static dbg-static stldbg-static

ifneq ($(_LSUPCPP_OBJ),"")
$(_LSUPCPP_AUX_TSMP):	$(_LSUPCPP)
	if [ ! -d $(AUX_DIR) ]; then mkdir -p $(AUX_DIR); fi
	cd $(AUX_DIR); $(AR) xo $(_LSUPCPP) && touch -r $(_LSUPCPP) $(_LSUPCPP_TSMP)
endif

release-static: $(OUTPUT_DIR_A) ${A_NAME_OUT}

dbg-static:	$(OUTPUT_DIR_A_DBG) ${A_NAME_OUT_DBG}

stldbg-static:	$(OUTPUT_DIR_A_STLDBG) ${A_NAME_OUT_STLDBG}

${A_NAME_OUT}:	$(OBJ_A) $(_LSUPCPP_AUX_TSMP)
	rm -f $@
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A) $(_LSUPCPP_AUX_OBJ)

${A_NAME_OUT_DBG}:	$(OBJ_A_DBG) $(_LSUPCPP_AUX_TSMP)
	rm -f $@
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_DBG) $(_LSUPCPP_AUX_OBJ)

${A_NAME_OUT_STLDBG}:	$(OBJ_A_STLDBG) $(_LSUPCPP_AUX_TSMP)
	rm -f $@
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_STLDBG) $(_LSUPCPP_AUX_OBJ)
