# $Id: makefile,v 1.9 2001/08/21 20:13:17 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = smss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

TARGET_OBJECTS = $(TARGET_NAME).o init.o smapi.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
