PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rossym

include $(PATH_TO_TOP)/config

TARGET_CFLAGS = -Wall -Werror -ffreestanding -D__USE_W32API

TARGET_OBJECTS = \
	delete.o \
	find.o \
	fromfile.o \
	frommem.o \
	fromraw.o \
	getraw.o \
	init.o \
        initkm.o \
        initum.o \
	zwfile.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/tools/depend.mk

# EOF
