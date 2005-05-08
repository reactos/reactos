# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = advpack

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_ACLEDIT)

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

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a advapi32.a ole32.a version.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = advpack.o stubs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

