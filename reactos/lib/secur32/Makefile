# $Id$


PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = secur32

TARGET_BASE = $(TARGET_BASE_LIB_SECURE32)

TARGET_SDKLIBS = ntdll.a

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -D__SECUR32__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_OBJECTS = lsa.o dllmain.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
