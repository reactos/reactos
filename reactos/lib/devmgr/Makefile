# $Id: Makefile,v 1.1 2004/04/04 21:49:15 weiden Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = devmgr

TARGET_INSTALLDIR = system32

TARGET_BASE = 0x72A90000

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a user32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = devmgr.o stubs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
