# $Id: makefile,v 1.3 2004/09/22 22:25:12 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = no

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = mdi

TARGET_SDKLIBS = kernel32.a gdi32.a ntdll.a comctl32.a comdlg32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
