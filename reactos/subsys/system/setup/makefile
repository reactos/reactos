# $Id: makefile,v 1.4 2004/08/18 02:16:00 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = setup

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = kernel32.a ole32.a shell32.a shlwapi.a

TARGET_GCCLIBS = uuid

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -D_WIN32_IE=0x0400

TARGET_LFLAGS =

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF