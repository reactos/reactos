
PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = commctrl

TARGET_BASE=0x75e60000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = commctrl_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
