# $Id: Makefile,v 1.1 2003/11/24 14:25:28 weiden Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = msgina

TARGET_BASE = 0x75970000

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -D__REACTOS__ \
 -Wall \
 -Werror \
 -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = msgina.o stubs.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
