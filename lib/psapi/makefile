# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psapi

TARGET_SDKLIBS = pseh.a epsapi.a ntdll.a kernel32.a

TARGET_CFLAGS = -I./include -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D_SEH_NO_NATIVE_NLG

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_BASE = $(TARGET_BASE_LIB_PSAPI)

TARGET_PCH = precomp.h

TARGET_OBJECTS = malloc.o psapi.o
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
