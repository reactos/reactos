# $Id: makefile,v 1.36 2001/11/02 22:47:36 hbirr Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = vfatfs

TARGET_OBJECTS = \
  blockdev.o \
  cleanup.o \
  close.o \
  create.o \
  dir.o \
  direntry.o \
  dirwr.o \
  fat.o \
  fcb.o \
  finfo.o \
  iface.o \
  rw.o \
  shutdown.o \
  string.o \
  volume.o \
  misc.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

