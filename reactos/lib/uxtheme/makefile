# $Id: makefile,v 1.1 2003/10/24 21:29:48 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = uxtheme

TARGET_SDKLIBS = kernel32.a user32.a gdi32.a

TARGET_CFLAGS = -Wall -Werror -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_BASE = 0x5B180000

TARGET_OBJECTS = \
 uxthemedll.o \
 button.o 
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
