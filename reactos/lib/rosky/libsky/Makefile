# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = libsky

TARGET_INSTALLDIR = system32

TARGET_BASE = 0x71c10000

TARGET_CFLAGS = \
 -I./include \
 -D__USE_W32API \
 -DDEBUG \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_ENTRY = 0x00000000

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = libsky.o stubs.o

TARGET_DEFONLY = yes

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
