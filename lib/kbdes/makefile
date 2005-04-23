PATH_TO_TOP = ../..

TARGET_TYPE = kmdll

TARGET_NAME = kbdes

TARGET_BASE = 0x5500000

TARGET_CFLAGS = -I$(PATH_TO_TOP)/ntoskrnl/include

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = kbdes.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
