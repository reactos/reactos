# $Id: makefile,v 1.3 2003/07/11 20:55:43 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

THREAD_OBJECTS = \
 thread/context.o \
 thread/create.o \
 thread/stack.o

MISC_OBJECTS = \
 misc/qsort.o

TARGET_OBJECTS = $(THREAD_OBJECTS) $(MISC_OBJECTS)
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
