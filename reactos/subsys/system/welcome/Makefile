# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = welcome

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -Wall -Werror -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API

TARGET_SDKLIBS = kernel32.a gdi32.a user32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
