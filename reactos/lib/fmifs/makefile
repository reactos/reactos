# $Id: makefile,v 1.22 2004/05/29 21:24:43 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = fmifs

TARGET_BASE = $(TARGET_BASE_LIB_FMIFS)

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
