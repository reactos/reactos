# $Id: Makefile,v 1.3 2004/01/09 21:54:24 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = syssetup

TARGET_BASE = 0x74A30000

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a userenv.a

TARGET_CFLAGS = -Wall -Werror -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS
 
TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_OBJECTS = dllmain.o install.o logfile.o


include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

# EOF
