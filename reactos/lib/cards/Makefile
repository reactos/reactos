PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = cards

TARGET_BASE = $(TARGET_BASE_LIB_CARDS) 

TARGET_CFLAGS = -fno-builtin -D__USE_W32API

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -DUNICODE -D_UNICODE -Wall -Werror

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = gdi32.a user32.a

TARGET_OBJECTS = cards.o 

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
