# $Id: makefile,v 1.15 2003/02/02 19:24:15 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = fmifs

TARGET_BASE = 0x76df0000

TARGET_CFLAGS = -fno-builtin

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = ntdll.a kernel32.a

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
