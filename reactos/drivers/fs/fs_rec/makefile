# $Id: makefile,v 1.2 2002/09/23 20:09:10 guido Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = fs_rec

TARGET_OBJECTS = \
  blockdev.o \
  fs_rec.o \
  cdfs.o \
  fat.o \
  ntfs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES) *.o *.sys *.sym

TARGET_CFLAGS = -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
