# $Id: Makefile,v 1.5 2003/12/21 14:38:25 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = welcome

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -Wall -Werror

TARGET_SDKLIBS = kernel32.a gdi32.a user32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
