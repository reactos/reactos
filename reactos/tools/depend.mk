# Automatic dependancy tracking
# Define $DEP_OBJECTS before this file is included
# $DEP_OBJECTS contain a list of object files that are checked for dependancies

ifneq ($(DEPENDENCIES),no)
ifneq ($(MAKECMDGOALS),implib)

DEP_FILTERED := $(filter-out $(DEP_EXCLUDE_FILTER), $(DEP_OBJECTS:.o=.d))

PCH :=

ifeq ($(ROS_USE_PCH),yes)
ifneq ($(TARGET_PCH),)

PCH = $(TARGET_PCH).gch

DEP_FILTERED := $(DEP_FILTERED) $(TARGET_PCH:.h=.d) 

endif	# TARGET_PCH
endif

DEP_FILES := $(join $(dir $(DEP_FILTERED)), $(addprefix ., $(notdir $(DEP_FILTERED))))
 
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(DEP_FILES),)
-include $(DEP_FILES)
endif
endif

ifeq ($(SEP),\)
DEPENDS_PATH := $(subst /,\,$(PATH_TO_TOP))\tools
else
DEPENDS_PATH := $(PATH_TO_TOP)/tools
endif

.%.d: %.c $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.cc $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.cpp $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.S  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.s  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.asm $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(NASM_CMD) $(NFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.h $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES) 
	$(HALFVERBOSEECHO) [DEPENDS] $@
	$(PCH_CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

endif
endif
