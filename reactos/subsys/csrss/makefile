# $Id: makefile,v 1.23 2003/10/20 18:02:04 gvg Exp $

PATH_TO_TOP = ../..

#TARGET_TYPE = program
TARGET_TYPE = proglib

TARGET_APPTYPE = native

TARGET_NAME = csrss

#TARGET_SDKLIBS = kernel32.a gdi32.a user32.a ntdll.a

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__ -Wall -Werror -Iinclude

TARGET_LFLAGS = -nostdlib

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

SUBDIRS = usercsr

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

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

#all::
#	$(MAKE) -C usercsr all
#
#clean::
#	$(MAKE) -C usercsr clean
#
#install::
#	$(MAKE) -C usercsr install
#
#dist::
#	$(MAKE) -C usercsr dist
#
#bootcd::
#	$(MAKE) -C usercsr bootcd

# EOF
