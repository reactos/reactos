# $Id: makefile,v 1.2 2003/11/14 17:13:33 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = setup

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = kernel32.a

TARGET_CFLAGS = -DUNICODE -Wall -Werror

TARGET_LFLAGS =

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF