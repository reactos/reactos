# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = lsasrv

TARGET_BASE = $(TARGET_BASE_LIB_LSASRV)

TARGET_ENTRY = 0x0

TARGET_CFLAGS += -Wall -Werror

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = lsasrv.o lsaport.o

DEP_OBJECTS = $(TARGET_OBJECTS)


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
