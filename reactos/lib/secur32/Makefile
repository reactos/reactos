# $Id: Makefile,v 1.10 2003/02/02 19:24:15 hyperion Exp $


PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = secur32

TARGET_BASE = 0x10000000

TARGET_SDKLIBS = ntdll.a

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -D__SECUR32__

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_OBJECTS = lsa.o dllmain.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
