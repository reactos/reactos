# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winmm

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = -D__WINE_FOR_REACTOS__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = \
	winmm.o \
	lolvldrv.o \
	driver.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
