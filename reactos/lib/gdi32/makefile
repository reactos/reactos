# $Id: makefile,v 1.42 2004/12/30 03:42:03 navaraf Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = gdi32

TARGET_BASE = $(TARGET_BASE_LIB_GDI32)

TARGET_PCH = include/precomp.h

TARGET_SDKLIBS = rosrtl.a ntdll.a advapi32.a kernel32.a

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -Wall \
 -Werror \
 -fno-builtin \
 -D__USE_W32API \
 -DWINVER=0x0600 \
 -D_WIN32_WINNT=0x0501

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

MAIN_OBJECTS = main/dllmain.o

MISC_OBJECTS = \
 misc/heap.o \
 misc/misc.o \
 misc/stubs.o \
 misc/stubsa.o \
 misc/stubsw.o \
 misc/wingl.o \
 misc/win32k.o

OBJECTS_OBJECTS = \
 objects/bitblt.o \
 objects/dc.o \
 objects/font.o \
 objects/linedda.o \
 objects/region.o \
 objects/metafile.o \
 objects/text.o

TARGET_OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(OBJECTS_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

DEP_EXCLUDE_FILTER = misc/win32k.%

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
