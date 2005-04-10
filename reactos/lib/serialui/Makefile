
PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = serialui

TARGET_BASE = $(TARGET_BASE_LIB_SERIALUI)

TARGET_CFLAGS = \
	-I./include \
	-Wall \
	-Werror \
	-fno-builtin \
	-DUNICODE \
	-DLE \
	-DDBG \
	-D__USE_W32API

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = kernel32.a user32.a shlwapi.a

TARGET_OBJECTS = serialui.o

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
