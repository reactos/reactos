# $Id: makefile,v 1.2 2003/11/14 17:13:32 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_SDKLIBS = ntdll.a fmifs.a

TARGET_NAME = format

TARGET_CFLAGS = -Werror -Wall -Wno-format

TARGET_OBJECTS = \
	$(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
