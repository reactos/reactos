# $Id: Makefile,v 1.1 2004/06/18 20:43:44 kuehng Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_NAME = control

TARGET_INSTALLDIR = system32

TARGET_APPTYPE = windows

MK_CFLAGS = -D_UNICODE -DUNICODE
MK_CPPFLAGS = -D_UNICODE -DUNICODE
MK_RCFLAGS = -D_UNICODE -DUNICODE

TARGET_SDKLIBS = kernel32.a user32.a comctl32.a

# TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = control.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
