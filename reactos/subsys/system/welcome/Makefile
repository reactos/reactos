# $Id: Makefile,v 1.4 2003/11/16 21:46:46 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = welcome

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API

TARGET_SDKLIBS = kernel32.a gdi32.a user32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
