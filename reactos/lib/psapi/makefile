# $Id: makefile,v 1.7 2003/04/14 01:19:07 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psapi

TARGET_SDKLIBS = epsapi.a ntdll.a kernel32.a

TARGET_CFLAGS = -I./include

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_BASE = 0x68F70000

TARGET_OBJECTS = \
	misc/dllmain.o \
	misc/malloc.o \
	misc/stubs.o \
	misc/win32.o
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
