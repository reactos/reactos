# $Id: makefile,v 1.4 2004/01/13 00:01:52 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = no

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = notepad

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

TARGET_SDKLIBS = comdlg32.a shell32.a user32.a gdi32.a

TARGET_OBJECTS = \
	License_En.o \
	dialog.o \
	license.o \
	main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
