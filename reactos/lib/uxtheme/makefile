# $Id: makefile,v 1.2 2003/10/25 00:46:01 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = uxtheme

TARGET_SDKLIBS = kernel32.a user32.a gdi32.a

TARGET_CFLAGS = -Wall -Werror -D_DISABLE_TIDENTS -D__USE_W32API -I./include

TARGET_LFLAGS = -nostartfiles --enable-stdcall-fixup

TARGET_BASE = 0x5B180000

TARGET_OBJECTS = \
 uxthemedll.o \
 nostyle/button.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
