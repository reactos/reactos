# $Id: Makefile,v 1.1 2004/08/12 02:50:35 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = libskygi

TARGET_INSTALLDIR = system32

TARGET_BASE = 0x71b10000

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_ENTRY = 0x00000000

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = libskygi.o stubs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
