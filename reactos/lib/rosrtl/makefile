# $Id: makefile,v 1.2 2003/05/29 00:36:41 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

THREAD_OBJECTS = \
 thread/context.o \
 thread/create.o \
 thread/stack.o

TARGET_OBJECTS = $(THREAD_OBJECTS)
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
