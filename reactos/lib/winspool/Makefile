# $Id: Makefile,v 1.1 2003/04/30 20:44:06 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winspool

TARGET_EXTENSION = .drv

TARGET_BASE = 0x77800000

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DLE \
 -DDBG

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = kernel32.a

TARGET_OBJECTS = stubs.o

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
