# $Id$

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

TARGET_CFLAGS = -Werror -Wall

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
