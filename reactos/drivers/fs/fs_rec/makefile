# $Id: makefile,v 1.4 2003/11/13 15:25:28 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = fs_rec

TARGET_OBJECTS = \
  blockdev.o \
  fs_rec.o \
  cdfs.o \
  fat.o \
  ntfs.o \
  udfs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES) *.o *.sys *.sym

TARGET_CFLAGS = -Werror -Wall

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
