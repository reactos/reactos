
PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = msvcrt20

TARGET_BASE = $(TARGET_BASE_LIB_MSVCRT20)

TARGET_LFLAGS = -nostartfiles --enable-stdcall-fixup

TARGET_SDKLIBS = libwine.a string.a kernel32.a ntdll.a

TARGET_GCCLIBS = gcc

TARGET_CFLAGS = -D__USE_W32API -D_MSVCRT_LIB_ -Wall

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += \
	-D_DISABLE_TIDENTS \
	-D__USE_W32API \
	-D__REACTOS__ \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 \
	-DUSE_MSVCRT_PREFIX \
	-D_MT

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
