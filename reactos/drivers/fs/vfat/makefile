# $Id: makefile,v 1.40 2002/08/14 20:58:31 dwelch Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = vfatfs

TARGET_GCCLIBS = gcc

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
  misc.o \
  fsctl.o \
  ea.o

DEP_OBJECTS = $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES) *.o *.sys *.sym

TARGET_CFLAGS = -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

