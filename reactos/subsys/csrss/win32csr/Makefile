# $Id: Makefile,v 1.1 2003/12/02 11:38:46 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = win32csr

TARGET_BASE = 0x5ffb0000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror -I../include

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a

TARGET_OBJECTS = dllmain.o conio.o guiconsole.o

TARGET_ENTRY = _DllMain@12

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
