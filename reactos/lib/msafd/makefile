# $Id: makefile,v 1.11 2003/09/12 17:51:47 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = msafd

TARGET_BASE = 0x777a0000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

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
