# $Id: makefile,v 1.16 2003/04/05 23:17:21 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = fmifs

TARGET_BASE = 0x76df0000

TARGET_CFLAGS = -fno-builtin

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_LIBS = $(PATH_TO_TOP)/lib/fslib/vfatlib/vfatlib.a

TARGET_OBJECTS = \
  chkdsk.o \
  compress.o \
  diskcopy.o \
  dllmain.o \
  extend.o \
	format.o \
	init.o \
	label.o \
	media.o 
	
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

TARGET_FILES = misc/*.o

# EOF
