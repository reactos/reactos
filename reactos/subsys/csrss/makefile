# $Id: makefile,v 1.28 2004/04/09 20:03:15 navaraf Exp $

PATH_TO_TOP = ../..

#TARGET_TYPE = program
TARGET_TYPE = proglib

TARGET_APPTYPE = native

TARGET_NAME = csrss

TARGET_SDKLIBS = rosrtl.a ntdll.a

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__ -Wall -Werror -Iinclude

TARGET_LFLAGS = -nostdlib

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D__USE_W32API -DWINVER=0x0600 -D_WIN32_WINNT=0x0501

SUBDIRS = win32csr

OBJECTS_API = \
  api/process.o \
  api/wapi.o \
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

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
