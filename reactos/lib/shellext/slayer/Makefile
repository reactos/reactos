# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = slayer

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_EXT_SLAYER)

TARGET_CFLAGS = \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = kernel32.a advapi32.a user32.a gdi32.a wine_uuid.a ole32.a \
                 shell32.a comctl32.a comdlg32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = precomp.h

TARGET_CLEAN = 

TARGET_OBJECTS = slayer.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
