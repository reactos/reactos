# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rosrtl

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

THREAD_OBJECTS = \
 thread/create.o \
 thread/exit.o \
 thread/stack.o \
 thread/priv.o

STRING_OBJECTS = \
 string/append.o \
 string/resstr.o

REGISTRY_OBJECTS = \
 registry/registry.o

MISC_OBJECTS = \
 misc/devmode.o \
 misc/intrlck.o \
 misc/logfont.o \
 misc/qsort.o

FILE_OBJECTS = \
 file/sparse.o

RECMUTEX_OBJECTS = recmutex/recmutex.o

include $(PATH_TO_TOP)/config

include makefile.$(ARCH)

TARGET_TYPE = library

TARGET_NAME = rosrtl

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

TARGET_OBJECTS = $(THREAD_OBJECTS) $(MISC_OBJECTS) $(STRING_OBJECTS) \
		 $(REGISTRY_OBJECTS) $(FILE_OBJECTS) $(RECMUTEX_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
