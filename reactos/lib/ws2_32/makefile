# $Id: makefile,v 1.17 2004/10/23 21:05:11 chorns Exp $

PATH_TO_TOP = ../..

TARGET_REGTESTS = yes

TARGET_TYPE = dynlink

TARGET_NAME = ws2_32

TARGET_PCH = include/ws2_32.h

TARGET_BASE = $(TARGET_BASE_LIB_W2_32)

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DLE \
 -DDBG \
 -D__USE_W32API

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = \
  misc/bsd.o misc/catalog.o misc/dllmain.o \
  misc/event.o misc/handle.o misc/ns.o \
  misc/sndrcv.o misc/stubs.o misc/upcall.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
