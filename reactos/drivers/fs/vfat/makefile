# $Id: makefile,v 1.44 2004/08/01 21:57:18 navaraf Exp $

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
TARGET_CFLAGS = -Wall -Werror -Wno-strict-aliasing -O6
endif

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

