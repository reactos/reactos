# $Id$

PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = maze

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = maze.o

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
