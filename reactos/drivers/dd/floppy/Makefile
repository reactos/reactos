# $Id: Makefile,v 1.12 2003/11/13 14:22:03 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = floppy

TARGET_OBJECTS = \
  dpc.o \
  floppy.o \
  isr.o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
