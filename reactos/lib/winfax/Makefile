# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winfax

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_WINFAX)

TARGET_CFLAGS = \
 -D__USE_W32API \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D_WIN32_IE=0x0500 \
 -D_WIN32_WINNT=0x501 \
 -DWINVER=0x600 \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a rosrtl.a kernel32.a comctl32.a user32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = precomp.h

TARGET_CLEAN = 

TARGET_OBJECTS = winfax.o

TARGET_ENTRY = 0x00000000

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
