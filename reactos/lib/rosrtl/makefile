# $Id: makefile,v 1.10 2003/12/30 05:10:32 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

THREAD_OBJECTS = \
 thread/create.o \
 thread/exit.o \
 thread/stack.o

MISC_OBJECTS = \
 misc/devmode.o \
 misc/logfont.o \
 misc/qsort.o

include $(PATH_TO_TOP)/config

include makefile.$(ARCH)

TARGET_TYPE = library

TARGET_NAME = rosrtl

TARGET_CFLAGS = -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = $(THREAD_OBJECTS) $(MISC_OBJECTS)
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
