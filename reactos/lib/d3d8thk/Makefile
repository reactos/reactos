PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = d3d8thk

TARGET_BASE = $(TARGET_BASE_LIB_D3D8THK)

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

TARGET_SDKLIBS = ntdll.a

TARGET_OBJECTS = win32k.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
