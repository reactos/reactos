# $Id: Makefile,v 1.1 2004/08/10 00:09:40 weiden Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = aclui

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_ACLUI)

TARGET_CFLAGS = \
 -I./include \
 -D__USE_W32API \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D_WIN32_WINNT=0x501 \
 -DWINVER=0x600 \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a user32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = aclui.o stubs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
