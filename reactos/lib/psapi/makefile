# $Id: makefile,v 1.12 2004/11/03 22:42:59 weiden Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psapi

TARGET_SDKLIBS = epsapi.a ntdll.a kernel32.a

TARGET_CFLAGS = -I./include -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_BASE = $(TARGET_BASE_LIB_PSAPI)

TARGET_PCH = precomp.h

TARGET_OBJECTS = \
	enum/drivers.o \
	enum/modules.o \
	enum/processes.o \
	misc/dllmain.o \
	misc/malloc.o \
	misc/stubs.o \
	misc/win32.o
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
