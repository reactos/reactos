# Automatic dependancy tracking
# Define $DEP_OBJECTS before this file is included
# $DEP_OBJECTS contain a list of object files that are checked for dependancies

ifneq ($(DEPENDENCIES),no)

DEP_FILTERED := $(filter-out $(DEP_EXCLUDE_FILTER), $(DEP_OBJECTS:.o=.d))
DEP_FILES := $(join $(dir $(DEP_FILTERED)), $(addprefix ., $(notdir $(DEP_FILTERED))))
 
ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

ifeq ($(SEP),\)
DEPENDS_PATH := $(subst /,\,$(PATH_TO_TOP))\tools
else
DEPENDS_PATH := $(PATH_TO_TOP)/tools
endif

MAKEDEP := $(DEPENDS_PATH)$(SEP)makedep$(EXE_POSTFIX)

.%.d: %.c $(MAKEDEP) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

.%.d: %.cc $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

.%.d: %.cpp $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

.%.d: %.s  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

.%.d: %.S  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

.%.d: %.asm $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX) $(GENERATED_HEADER_FILES)
	$(MAKEDEP) $(filter -I%, $(CFLAGS)) -f$@ $<

endif
