PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = mmdrv

# -fno-builtin
TARGET_CFLAGS = -D__USE_W32API

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -DUNICODE -D_UNICODE -Wall -Werror

TARGET_LFLAGS =

TARGET_SDKLIBS = winmm.a

TARGET_BASE = $(TARGET_BASE_LIB_MMDRV)

TARGET_OBJECTS = \
	auxil.o \
	entry.o \
	midi.o \
	utils.o \
	wave.o
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
