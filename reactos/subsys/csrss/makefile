# $Id: makefile,v 1.22 2003/08/07 04:03:24 royce Exp $

PATH_TO_TOP = ../..

#TARGET_TYPE = program
TARGET_TYPE = proglib

TARGET_APPTYPE = native

TARGET_NAME = csrss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

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
  video.o \
  win32k.o

TARGET_OBJECTS = \
  $(OBJECTS_API) \
  $(OBJECTS_MISC)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
