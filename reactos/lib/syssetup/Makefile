# $Id: Makefile,v 1.1 2003/05/02 18:07:55 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = syssetup

TARGET_BASE = 0x74A30000

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = -Wall -Werror -fno-builtin
 
TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_OBJECTS = dllmain.o install.o logfile.o


include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

# EOF
