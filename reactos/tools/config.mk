# Global configuration

#
# Include details of the OS configuration
#
include $(PATH_TO_TOP)/config

CONFIG :=

ifeq ($(DBG), 1)
CONFIG += DBG
endif

ifeq ($(KDBG), 1)
CONFIG += KDBG
endif

ifeq ($(MP), 1)
CONFIG += MP
else
CONFIG += UP
endif

ifeq ($(ACPI), 1)
CONFIG += ACPI
endif

ifeq ($(SEH), 1)
CONFIG += SEH
endif


$(PATH_TO_TOP)/tools/mkconfig$(EXE_POSTFIX): $(PATH_TO_TOP)/tools/mkconfig.c
	$(HOST_CC) -g -o $(PATH_TO_TOP)/tools/mkconfig$(EXE_POSTFIX) $(PATH_TO_TOP)/tools/mkconfig.c

$(PATH_TO_TOP)/config: $(PATH_TO_TOP)/tools/mkconfig$(EXE_POSTFIX)
	$(PATH_TO_TOP)/tools/mkconfig$(EXE_POSTFIX) $(PATH_TO_TOP)/include/roscfg.h$(CONFIG)

$(PATH_TO_TOP)/include/roscfg.h: $(PATH_TO_TOP)/config
	$(PATH_TO_TOP)/tools/mkconfig$(EXE_POSTFIX) $(PATH_TO_TOP)/include/roscfg.h$(CONFIG)
