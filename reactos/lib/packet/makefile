# $Id: makefile,v 1.7 2004/05/29 21:24:46 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = packet

TARGET_BASE = $(TARGET_BASE_LIB_PACKET)

TARGET_CFLAGS = -I./include -DUNICODE -D_UNICODE -DLE -DDBG -D_DEBUG -D__USE_W32API -Wall -Werror

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = \
	Packet32.o \
	trace.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
