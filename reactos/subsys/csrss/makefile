# $Id: makefile,v 1.15 2001/08/21 20:13:16 chorns Exp $

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
  api/handle.o

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
