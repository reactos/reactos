# $Id: makefile,v 1.14 2004/06/09 18:11:39 arty Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = msafd

TARGET_BASE = $(TARGET_BASE_LIB_MSAFD)

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DDBG \
 -D__USE_W32API

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
