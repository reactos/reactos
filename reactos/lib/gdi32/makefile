# $Id: makefile,v 1.36 2004/03/23 00:18:54 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = gdi32

TARGET_BASE = 0x77ed0000

TARGET_SDKLIBS = rosrtl.a ntdll.a advapi32.a kernel32.a

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -Wall \
 -Werror \
 -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

MAIN_OBJECTS = main/dllmain.o

MISC_OBJECTS = \
 misc/heap.o \
 misc/stubs.o \
 misc/stubsa.o \
 misc/stubsw.o \
 misc/wingl.o \
 misc/win32k.o

OBJECTS_OBJECTS = \
 objects/bitblt.o \
 objects/brush.o \
 objects/clip.o \
 objects/dc.o \
 objects/fillshap.o \
 objects/font.o \
 objects/line.o \
 objects/linedda.o \
 objects/metafile.o \
 objects/pen.o \
 objects/region.o \
 objects/text.o

TARGET_OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(OBJECTS_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

DEP_EXCLUDE_FILTER = misc/win32k.%

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
