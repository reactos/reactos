# $Id: makefile,v 1.2 2004/02/14 20:13:04 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API

TARGET_NAME = user32_test

TARGET_SDKLIBS = gdi32.a user32.a

TARGET_OBJECTS = \
	testlist.o \
	class.o \
	dialog.o \
	listbox.o \
	msg.o \
	resource.o \
	sysparams.o \
	win.o \
	wsprintf.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
