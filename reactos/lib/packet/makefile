# $Id: makefile,v 1.3 2002/09/24 15:08:14 robd Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = packet

TARGET_BASE = 0x77780000

TARGET_CFLAGS = -I./include -DUNICODE -D_UNICODE -DLE -DDBG -D_DEBUG

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = \
	Packet32.o \
	trace.o

TARGET_CLEAN = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
