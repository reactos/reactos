# $Id: makefile,v 1.8 2002/09/07 15:12:08 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = wshtcpip

TARGET_BASE = 0x77200000

TARGET_CFLAGS = -DUNICODE

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
