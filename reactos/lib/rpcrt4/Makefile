
PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = rpcrt4

TARGET_BASE=0x76380000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API

TARGET_SDKLIBS = ntdll.a winedbgc.a

TARGET_OBJECTS = \
rpcrt4_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
