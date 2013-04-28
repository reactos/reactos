# -*- makefile -*- Time-stamp: <03/10/13 17:47:13 ptr>
# $Id$

# Shared libraries tags

release-shared:	$(OUTPUT_DIR) $(SO_NAME_OUT)

dbg-shared:	$(OUTPUT_DIR_DBG) $(SO_NAME_OUT_DBG)

stldbg-shared:	$(OUTPUT_DIR_STLDBG) $(SO_NAME_OUT_STLDBG)

# .o{$(OUTPUT_DIR)}.o:

$(SO_NAME_OUT):	$(OBJ) $(RES) $(LIBSDEP)
	$(LINK_cc_REL) $(LINK_OUTPUT_OPTION) $(OBJ) $(RES) $(LDLIBS)
	@if exist $(MANIFEST_NAME_OUT) mt /nologo /manifest $(MANIFEST_NAME_OUT) /outputresource:$(SO_NAME_OUT);#2

$(SO_NAME_OUT_DBG):	$(OBJ_DBG) $(RES_DBG) $(LIBSDEP)
	$(LINK_cc_DBG) $(LINK_OUTPUT_OPTION) $(OBJ_DBG) $(RES_DBG) $(LDLIBS)
	@if exist $(MANIFEST_NAME_OUT_DBG) mt /nologo /manifest $(MANIFEST_NAME_OUT_DBG) /outputresource:$(SO_NAME_OUT_DBG);#2

$(SO_NAME_OUT_STLDBG):	$(OBJ_STLDBG) $(RES_STLDBG) $(LIBSDEP)
	$(LINK_cc_STLDBG) $(LINK_OUTPUT_OPTION) $(OBJ_STLDBG) $(RES_STLDBG) $(LDLIBS)
	@if exist $(MANIFEST_NAME_OUT_STLDBG) mt /nologo /manifest $(MANIFEST_NAME_OUT_STLDBG) /outputresource:$(SO_NAME_OUT_STLDBG);#2

