# $Id: makefile,v 1.10 2003/08/07 04:03:24 royce Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = smss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = $(TARGET_NAME).o init.o smapi.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
