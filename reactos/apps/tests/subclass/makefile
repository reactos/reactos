# $Id: makefile,v 1.1 2003/11/08 22:10:15 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = subclass

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS += -D_DISABLE_TIDENTS

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
