# $Id: makefile,v 1.1 2002/08/27 00:53:34 robd Exp $

PATH_TO_TOP = ../../../reactos

TOOLS_PATH = $(PATH_TO_TOP)/tools

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = dialog

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a

TARGET_GCCLIBS = advapi32 comctl32 comdlg32 version

TARGET_OBJECTS = $(TARGET_NAME).o \
	memdlg.o \
	page1.o \
	page2.o \
	page3.o \
	trace.o

TARGET_CLEAN = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

MK_CFLAGS = -D_UNICODE -DUNICODE -D_DEBUG
MK_CPPFLAGS = -D_UNICODE -DUNICODE -D_DEBUG
MK_RCFLAGS = -D_UNICODE -DUNICODE

# EOF
