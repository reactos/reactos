# $Id: Makefile,v 1.13 2003/06/21 19:55:55 hbirr Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = npfs

TARGET_OBJECTS = \
  create.o \
  finfo.o \
  fsctrl.o \
  npfs.o \
  rw.o \
  volume.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
