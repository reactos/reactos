# $Id: makefile,v 1.5 2003/07/21 02:36:00 royce Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

THREAD_OBJECTS = \
 thread/context.o \
 thread/create.o \
 thread/stack.o

MISC_OBJECTS = \
 misc/devmode.o \
 misc/qsort.o

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = $(THREAD_OBJECTS) $(MISC_OBJECTS)
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
