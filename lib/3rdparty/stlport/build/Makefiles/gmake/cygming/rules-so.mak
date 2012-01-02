# -*- makefile -*- Time-stamp: <03/10/27 18:15:05 ptr>
# $Id: rules-so.mak 3113 2007-08-08 19:05:21Z dums $

# Shared libraries tags

PHONY += release-shared dbg-shared stldbg-shared

release-shared:	$(OUTPUT_DIR) ${SO_NAME_OUT}

dbg-shared:	$(OUTPUT_DIR_DBG) ${SO_NAME_OUT_DBG}

stldbg-shared:	$(OUTPUT_DIR_STLDBG) ${SO_NAME_OUT_STLDBG}

ifeq (bcc, $(COMPILER_NAME))
# Borland linker
${SO_NAME_OUT}:	$(OBJ) $(RES) $(LIBSDEP)
	$(LINK.cc) $(subst /,\\,$(START_OBJ) $(OBJ) $(END_OBJ), $(LINK_OUTPUT_OPTION), , $(LDLIBS), , $(RES))
${SO_NAME_OUT_DBG}:	$(OBJ_DBG) $(RES_DBG) $(LIBSDEP)
	$(LINK.cc) $(subst /,\\,$(START_OBJ) $(OBJ_DBG) $(END_OBJ), $(LINK_OUTPUT_OPTION), , $(LDLIBS), , $(RES_DBG))
${SO_NAME_OUT_STLDBG}:	$(OBJ_STLDBG) $(RES_STLDBG) $(LIBSDEP)
	$(LINK.cc) $(subst /,\\,$(START_OBJ) $(OBJ_STLDBG) $(END_OBJ), $(LINK_OUTPUT_OPTION), , $(LDLIBS), , $(RES_STLDBG))
else
ifeq (dmc, $(COMPILER_NAME))
# Digital Mars linker
$(SO_NAME_OUT): $(OBJ) $(RES) $(LIBSDEP)
	@echo LIBRARY "$(SO_NAME_BASE).dll" > $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo DESCRIPTION \'STLport DLL for Digital Mars C/C++\'>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo EXETYPE NT>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo SUBSYSTEM WINDOWS>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo CODE SHARED EXECUTE>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo DATA READWRITE>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo EXPORTS>> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?cin@std@@3V?$$basic_istream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?cout@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?cerr@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?clog@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?wcin@std@@3V?$$basic_istream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?wcout@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?wcerr@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	@echo '?wclog@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR)/$(SO_NAME_BASE).def
	$(LINK.cc) $(subst /,\\,$(OBJ),$(LINK_OUTPUT_OPTION),$(MAP_OUTPUT_OPTION),$(LDLIBS),$(DEF_OPTION),$(RES))

$(SO_NAME_OUT_DBG): $(OBJ_DBG) $(RES_DBG) $(LIBSDEP)
	@echo LIBRARY "$(SO_NAME_DBG_BASE).dll" > $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo DESCRIPTION \'STLport DLL for Digital Mars C/C++\'>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo EXETYPE NT>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo SUBSYSTEM WINDOWS>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo CODE SHARED EXECUTE>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo DATA READWRITE>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo EXPORTS>> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?cin@std@@3V?$$basic_istream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?cout@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?cerr@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?clog@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?wcin@std@@3V?$$basic_istream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?wcout@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?wcerr@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	@echo '?wclog@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_DBG)/$(SO_NAME_DBG_BASE).def
	$(LINK.cc) $(subst /,\\,$(OBJ_DBG),$(LINK_OUTPUT_OPTION),$(MAP_OUTPUT_OPTION),$(LDLIBS),$(DEF_OPTION_DBG),$(RES_DBG))

$(SO_NAME_OUT_STLDBG):	$(OBJ_STLDBG) $(RES_STLDBG) $(LIBSDEP)
	@echo LIBRARY "$(SO_NAME_STLDBG_BASE).dll" > $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo DESCRIPTION \'STLport DLL for Digital Mars C/C++\'>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo EXETYPE NT>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo SUBSYSTEM WINDOWS>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo CODE SHARED EXECUTE>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo DATA READWRITE>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo EXPORTS>> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?cin@std@@3V?$$basic_istream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?cout@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?cerr@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?clog@std@@3V?$$basic_ostream@std@DV?$$char_traits@std@D@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?wcin@std@@3V?$$basic_istream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?wcout@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?wcerr@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	@echo '?wclog@std@@3V?$$basic_ostream@std@_YV?$$char_traits@std@_Y@1@@1@A' >> $(OUTPUT_DIR_STLDBG)/$(SO_NAME_STLDBG_BASE).def
	$(LINK.cc) $(subst /,\\,$(OBJ_STLDBG), $(LINK_OUTPUT_OPTION), $(MAP_OUTPUT_OPTION), $(LDLIBS), $(DEF_OPTION_STLDBG), $(RES_STLDBG))

else
# GNU linker
${SO_NAME_OUT}:	$(OBJ) $(RES) $(LIBSDEP) $(LSUPC++DEF)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ) $(RES) $(LSUPC++DEF) $(LDLIBS) $(STDLIBS)
${SO_NAME_OUT_DBG}:	$(OBJ_DBG) $(RES_DBG) $(LIBSDEP) $(LSUPC++DEF)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ_DBG) $(RES_DBG) $(LSUPC++DEF) $(LDLIBS) $(STDLIBS)
${SO_NAME_OUT_STLDBG}:	$(OBJ_STLDBG) $(RES_STLDBG) $(LIBSDEP) $(LSUPC++DEF)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ_STLDBG) $(RES_STLDBG) $(LSUPC++DEF) $(LDLIBS) $(STDLIBS)
endif
endif
