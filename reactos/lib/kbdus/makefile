# $Id: makefile,v 1.1 2003/07/31 16:47:08 royce Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = kbdus

TARGET_BASE = 0x5500000

TARGET_CFLAGS = -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS =

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = kbdus.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
