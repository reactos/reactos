# $Id$

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
TARGET_CFLAGS = -D__USE_W32API -Wall -Werror -g
else
TARGET_CFLAGS = -D__USE_W32API -Wall -Werror -O3 -fno-strict-aliasing 
endif

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

