# $Id: makefile,v 1.19 2002/10/28 15:19:09 robd Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = csrss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

OBJECTS_API = \
  api/process.o \
  api/wapi.o \
  api/conio.o \
  api/handle.o \
  api/user.o

OBJECTS_MISC = \
  $(TARGET_NAME).o \
  init.o \
  print.o \
  video.o

TARGET_OBJECTS = \
  $(OBJECTS_API) \
  $(OBJECTS_MISC)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
