PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = imagehlp

TARGET_CFLAGS = \
 -Wall \
 -fno-builtin

# Compile definitions usage:
#    __USE_W32API - Compilation with w32api headers
#    __REACTOS__ - Compilation of Wine sources for ReactOS
#    _WIN32_IE=0x600 - Internet Explorer 6 compatible defintions
#    WINVER=0x501 - Windows XP definitions
#    __need_offsetof - Force definition of macro offsetof in stddef.h
#
DEFINES = \
	-D_DISABLE_TIDENTS \
	-D__USE_W32API \
	-D__REACTOS__ \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 \
	-DWINVER=0x501 \
	-DCOBJMACROS \
	-D__need_offsetof
	
TARGET_CFLAGS += \
	$(DEFINES) \
	-DDCX_USESTYLE=0x10000L \
	-I$(PATH_TO_TOP)/include/wine

TARGET_RCFLAGS += -D__REACTOS__	-D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = libwine.a ntdll.a kernel32.a

TARGET_BASE = $(TARGET_BASE_LIB_IMAGEHLP)

TARGET_OBJECTS = \
debug.o \
imagehlp_main.o \
integrity.o \
modify.o \
symbol.o \
access.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# include $(TOOLS_PATH)/depend.mk
