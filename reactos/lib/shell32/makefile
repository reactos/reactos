# $Id: makefile,v 1.8 2003/10/04 21:30:05 navaraf Exp $

PATH_TO_TOP = ../..

TARGET_NAME = shell32

TARGET_TYPE = dynlink

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a advapi32.a msvcrt.a

TARGET_GCCLIBS = comctl32 comdlg32 version

TARGET_CFLAGS = -D_WIN32_IE=0x0400 -D__USE_W32API -DUNICODE -D_UNICODE -O2 -Wall -Wstrict-prototypes -fno-builtin

TARGET_CPPFLAGS = -D_WIN32_IE=0x0400 -D__USE_W32API -DUNICODE -D_UNICODE -O2 -Wall -Wstrict-prototypes -fno-builtin

TARGET_BASE = 0x77260000

MISC_OBJECTS = \
	misc/dllmain.o \
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
