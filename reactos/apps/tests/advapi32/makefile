# $Id: makefile,v 1.1 2004/08/07 22:51:27 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API

TARGET_NAME = advapi32_test

TARGET_SDKLIBS = advapi32.a

TARGET_OBJECTS = \
	testlist.o \
	registry.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
