# $Id: Makefile,v 1.6 2004/05/29 21:24:48 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = wsock32

TARGET_BASE = $(TARGET_BASE_LIB_WSOCK32)

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DLE \
 -DDBG \
 -D__USE_W32API

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = kernel32.a ws2_32.a mswsock.a

TARGET_OBJECTS = stubs.o

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
