PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = xboxvmp

TARGET_DDKLIBS = videoprt.a ntoskrnl.a

TARGET_CFLAGS = -Werror -Wall -I$(PATH_TO_TOP)/ntoskrnl/include -D__USE_W32API

TARGET_OBJECTS = \
  xboxvmp.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
