# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winmm

TARGET_BASE = 0x777c0000

WINE_RC = winmm_res

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

TARGET_SDKLIBS = ntdll.a kernel32.a libwine.a

TARGET_OBJECTS = \
	driver.o \
	joystick.o \
	lolvldrv.o \
	mci.o \
	mmio.o \
	playsound.o \
	time.o \
	winmm.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
