# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ws2help

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
