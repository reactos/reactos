# $Id: makefile,v 1.21 2004/02/23 11:55:12 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = fmifs

TARGET_BASE = 0x76df0000

TARGET_ENTRY = _InitializeFmIfs@12

TARGET_CFLAGS = -fno-builtin -Werror -Wall

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = vfatlib.a ntdll.a kernel32.a

TARGET_OBJECTS = \
	chkdsk.o \
	compress.o \
	diskcopy.o \
	extend.o \
	format.o \
	init.o \
	label.o \
	media.o 

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

TARGET_FILES = misc/*.o

# EOF
