# $Id: makefile,v 1.1 2004/01/23 10:33:21 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = samlib

TARGET_BASE = 0x777E0000

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_CFLAGS = -Wall -Werror -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS
 
TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_OBJECTS = dllmain.o samlib.o

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

# EOF
