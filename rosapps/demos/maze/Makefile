# $Id: Makefile,v 1.1 2003/08/15 17:06:49 gvg Exp $

PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = maze

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = maze.o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
