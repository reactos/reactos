# $Id: Makefile,v 1.6 2003/12/02 18:12:05 navaraf Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = twain_32

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_BASE=0x76380000

TARGET_SDKLIBS = ntdll.a libwine.a

TARGET_OBJECTS = \
	capability.o \
	ds_audio.o \
	ds_ctrl.o \
	ds_image.o \
	dsm_ctrl.o \
	twain32_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/config.mk
