# $Id: Makefile,v 1.1 2004/02/05 19:17:11 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_EXTENSION = .cpl

TARGET_NAME = cplsample

TARGET_INSTALLDIR = system32

TARGET_BASE = 0x75970000

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = kernel32.a user32.a comctl32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = cplsample.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
