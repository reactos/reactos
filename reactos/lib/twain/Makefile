# $Id: Makefile,v 1.4 2003/09/12 17:51:48 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = twain_32

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_BASE=0x76380000

TARGET_SDKLIBS = ntdll.a winedbgc.a

TARGET_OBJECTS = \
	capability.o \
	ds_audio.o \
	ds_ctrl.o \
	ds_image.o \
	dsm_ctrl.o \
	twain32_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
