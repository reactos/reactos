# $Id: Makefile,v 1.2 2004/11/07 18:41:19 weiden Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winscard

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_WINSCARD)

TARGET_CFLAGS = \
 -D__USE_W32API \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D_WIN32_IE=0x0500 \
 -D_WIN32_WINNT=0x502 \
 -DWINVER=0x600 \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a rosrtl.a kernel32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = precomp.h

TARGET_CLEAN = 

TARGET_OBJECTS = winscard.o

TARGET_ENTRY = 0x00000000

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
