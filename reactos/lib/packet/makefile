# $Id: makefile,v 1.2 2002/08/17 15:58:38 robd Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = packet

TARGET_BASE = 0x77780000

TARGET_CFLAGS = -I./include -DUNICODE -DLE -DDBG

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = \
    Packet32.o

TARGET_CLEAN = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
