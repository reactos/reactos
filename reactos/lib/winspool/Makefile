#
# Fix this
#

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
 -D__REACTOS__ \
 -D__USE_W32API \
 -DUNICODE \
 -DLE \
 -DDBG

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = libwine.a libwine_unicode.a ntdll.a kernel32.a

TARGET_OBJECTS = info.o stubs.o

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
