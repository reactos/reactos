# $Id: makefile,v 1.8 2003/07/22 20:10:04 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

THREAD_OBJECTS = \
 thread/create.o \
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

TARGET_OBJECTS = $(THREAD_OBJECTS) $(MISC_OBJECTS)
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
