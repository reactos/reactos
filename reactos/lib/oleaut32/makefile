# $Id: makefile,v 1.4 2003/08/07 04:03:23 royce Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = oleaut32

TARGET_BASE = 0x76260000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = dllmain.o stubs.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
