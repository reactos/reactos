# $Id:  $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = win32mu

TARGET_BASE = 0x5ffb0000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D__USE_W32API -D_DISABLE_TIDENTS -Wall -Werror -I../include

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a

TARGET_OBJECTS = dllmain.o init.o

TARGET_ENTRY = _DllMain@12

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
