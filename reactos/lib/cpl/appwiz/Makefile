# $Id: Makefile,v 1.2 2004/06/30 12:11:11 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_EXTENSION = .cpl

TARGET_NAME = appwiz

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_CPL_SYSDM)

TARGET_CFLAGS = \
 -D_WIN32_IE=0x0600 \
 -D_WIN32_WINNT=0x0501 \
 -I./include \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D__USE_W32API \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles


TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = kernel32.a user32.a comctl32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = appwiz.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
