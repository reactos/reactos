PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = riched32

TARGET_BASE = 0x76340000

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -DWM_ALTTABACTIVE=0x0029 -D_DISABLE_TIDENTS -D__USE_W32API

TARGET_SDKLIBS = ntdll.a winedbgc.a

TARGET_OBJECTS = charlist.o reader.o richedit.o text-writer.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
