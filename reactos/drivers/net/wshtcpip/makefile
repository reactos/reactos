# $Id: makefile,v 1.7 2001/08/21 20:13:16 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = wshtcpip

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = -DUNICODE

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
