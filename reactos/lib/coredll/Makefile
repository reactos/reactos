
PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = coredll

TARGET_BASE=$(TARGET_BASE_LIB_COREDLL)

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = coredll_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
