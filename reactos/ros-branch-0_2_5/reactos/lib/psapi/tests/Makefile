# $Id: Makefile,v 1.1 2004/11/07 20:05:22 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API -D_WIN32_IE=0x0600 \
	-D_WIN32_WINNT=0x0501 -D__REACTOS__

TARGET_NAME = psapi_test

TARGET_SDKLIBS = gdi32.a comctl32.a ntdll.a

TARGET_OBJECTS = \
	testlist.o \
	module.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF