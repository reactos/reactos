# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = iphlpapi

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = -DUNICODE

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

