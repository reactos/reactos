# $Id: makefile,v 1.43 2003/10/11 17:51:56 hbirr Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

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
  ea.o \
  flush.o

DEP_OBJECTS = $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES) *.o *.sys *.sym

include $(PATH_TO_TOP)/config

ifeq ($(DBG), 1)
TARGET_CFLAGS = -Wall -Werror -g
else
TARGET_CFLAGS = -Wall -Werror -fno-strict-aliasing -O6
endif

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

