# $Id: makefile,v 1.8 2003/12/01 18:21:04 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winlogon

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a secur32.a

TARGET_OBJECTS = $(TARGET_NAME).o setup.o wlx.o

TARGET_CFLAGS = -Wall -Werror -DUNICODE -I./ -I../../../include

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
