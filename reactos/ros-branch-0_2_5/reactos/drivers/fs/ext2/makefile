# $Id: makefile,v 1.14 2001/08/21 20:13:12 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = ext2fs

TARGET_CFLAGS = -I./include -DDBG

TARGET_OBJECTS = \
  attr.o \
  blockdev.o \
  dir.o \
  file.o \
  inode.o \
  rw.o \
  quota.o \
  security.o \
  super.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
