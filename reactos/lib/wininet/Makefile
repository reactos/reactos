# $Id: Makefile,v 1.1 2004/02/01 21:33:56 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = wininet

TARGET_BASE = 0x70200000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -Wall -Werror

TARGET_SDKLIBS = kernel32.a ntdll.a

TARGET_OBJECTS = stubs.o

TARGET_NORC = yes

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
