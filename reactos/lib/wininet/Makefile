# $Id: Makefile,v 1.2 2004/05/29 21:24:47 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = wininet

TARGET_BASE = $(TARGET_BASE_LIB_WININET)

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -Wall -Werror

TARGET_SDKLIBS = kernel32.a ntdll.a

TARGET_OBJECTS = stubs.o

TARGET_NORC = yes

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
