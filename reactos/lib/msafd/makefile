# $Id: makefile,v 1.9 2003/04/14 01:19:07 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = msafd

TARGET_BASE = 0x777a0000

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DDBG

TARGET_LFLAGS = -nostartfiles -nostdlib

MISC_OBJECTS = \
  misc/dllmain.o misc/event.o misc/helpers.o \
  misc/sndrcv.o misc/stubs.o

TARGET_OBJECTS = $(MISC_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
