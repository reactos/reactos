# $Id: makefile,v 1.11.22.1 2004/10/25 01:32:12 ion Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = wshtcpip

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = -DUNICODE -D__USE_W32API -Werror -Wall

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
