# $Id: makefile,v 1.7 2003/11/14 17:13:35 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winlogon

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a

TARGET_OBJECTS = $(TARGET_NAME).o setup.o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
