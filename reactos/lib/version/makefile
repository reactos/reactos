# $Id: makefile,v 1.13 2003/09/12 17:51:48 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = version

TARGET_BASE = 0x77a90000

TARGET_CFLAGS = -fno-builtin -Werror -Wall

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = kernel32.a ntdll.a

TARGET_OBJECTS =  misc/libmain.o misc/stubs.o

TARGET_CLEAN = misc/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
