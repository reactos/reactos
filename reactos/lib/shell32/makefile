# $Id: makefile,v 1.6 2002/09/24 15:06:09 robd Exp $

PATH_TO_TOP = ../..

SHELL32_ALIAS  = roshel32
COMCTL32_ALIAS = rosctl32
#SHELL32_ALIAS  = shell32
#COMCTL32_ALIAS = comctl32

#TARGET_NAME = $(SHELL32_ALIAS)
TARGET_NAME = roshel32

TARGET_DEFNAME = shell32

TARGET_TYPE = dynlink

#TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a advapi32.a $(COMCTL32_ALIAS).a msvcrt.a
TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a advapi32.a msvcrt.a

TARGET_GCCLIBS = comctl32 comdlg32 version

TARGET_CFLAGS = -D_WIN32_IE=0x0400 -DUNICODE -D_UNICODE -O2 -Wall -Wstrict-prototypes -fno-builtin -DDBG

TARGET_CPPFLAGS = -D_WIN32_IE=0x0400 -DUNICODE -D_UNICODE -O2 -Wall -Wstrict-prototypes -fno-builtin -DDBG

#TARGET_LFLAGS = -nostdlib

TARGET_BASE = 0x77260000

MISC_OBJECTS = \
	misc/dllmain.o \
	misc/_stubs.o \
	misc/stubs.o

CONTROL_OBJECTS = \
	control/framewnd.o \
	control/listview.o \
	control/control.o \
	control/settings.o \
	control/trace.o

TARGET_OBJECTS = \
	$(MISC_OBJECTS) \
	$(CONTROL_OBJECTS)

TARGET_CLEAN = $(TARGET_OBJECTS) $(TARGET_NAME).coff

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

MK_CFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE -I./ 
MK_CPPFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE -I./ 
MK_RCFLAGS = -D_WIN32_IE=0x0400 -D_UNICODE -DUNICODE

