# $Id: Makefile,v 1.3 2004/06/30 12:10:12 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_NAME = control

TARGET_INSTALLDIR = system32

TARGET_APPTYPE = windows

TARGET_CFLAGS = \
 -D_WIN32_IE=0x0600 \
 -D_WIN32_WINNT=0x0501 \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D__USE_W32API \
 -Wall \
 -Werror \
 -fno-builtin

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
