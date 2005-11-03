# $Id: Makefile,v 1.1 2004/10/21 04:48:46 sedwards Exp $

PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = zwcontinue

TARGET_OBJECTS = $(TARGET_NAME).o i386/$(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

TARGET_SDKLIBS = ntdll.a

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
