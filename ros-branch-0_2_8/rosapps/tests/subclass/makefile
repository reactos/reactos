# $Id: makefile,v 1.1 2004/10/21 04:48:45 sedwards Exp $

PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = subclass

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
