# $Id: makefile,v 1.11 2003/02/02 19:24:16 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = version

TARGET_BASE = 0x77a90000

TARGET_CFLAGS = -fno-builtin

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = kernel32.a ntdll.a

TARGET_OBJECTS =  misc/libmain.o misc/stubs.o

TARGET_CLEAN = misc/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
