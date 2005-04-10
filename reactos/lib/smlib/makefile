
PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = smlib

include $(PATH_TO_TOP)/config

TARGET_CFLAGS = -Wall -Werror -ffreestanding

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

ifneq ($(DBG), 0)
TARGET_CFLAGS += -DDBG
endif

TARGET_OBJECTS = \
	connect.o \
	execpgm.o \
	compses.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/tools/depend.mk

# EOF
