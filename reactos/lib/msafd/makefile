# $Id: makefile,v 1.8 2003/02/02 19:24:15 hyperion Exp $

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

TARGET_OBJECTS = $(TARGET_NAME).o 

TARGET_CLEAN = misc/*.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


MISC_OBJECTS = \
  misc/dllmain.o misc/event.o misc/helpers.o \
  misc/sndrcv.o misc/stubs.o

OBJECTS = $(MISC_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
