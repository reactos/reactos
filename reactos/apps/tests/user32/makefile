# $Id: makefile,v 1.3 2004/05/28 17:56:17 royce Exp $

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
	dde.o \
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
