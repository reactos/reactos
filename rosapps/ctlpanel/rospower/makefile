# $Id: makefile,v 1.1 2003/10/25 13:21:41 robd Exp $

PATH_TO_TOP = ../../../reactos

TOOLS_PATH = $(PATH_TO_TOP)/tools

TARGET_TYPE = dynlink

TARGET_NAME = rospower

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a

TARGET_GCCLIBS = advapi32 comctl32 comdlg32 version

# TARGET_CFLAGS = -DUNICODE

TARGET_EXTENSION = .cpl

TARGET_BASE = 0x77dc0000

TARGET_OBJECTS = \
	dllmain.o \
	main.o \
	system.o \
	power.o \
	trace.o

TARGET_CLEAN = %(TARGET_OBJECTS) $(TARGET_NAME).coff

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

MK_CFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE -D_DEBUG -I./ 
MK_CPPFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE -D_DEBUG -I./ 
MK_RCFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE -D_DEBUG 

