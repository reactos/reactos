# $Id: makefile,v 1.14 2001/08/21 20:13:06 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = fmifs

TARGET_BASE = 0x76df0000

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
