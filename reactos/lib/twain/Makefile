# $Id: Makefile,v 1.1 2003/07/22 04:00:28 sedwards Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = twain_32

TARGET_BASE=0x76380000

TARGET_SDKLIBS = ntdll.a

TARGET_OBJECTS = \
	capability.o \
	ds_audio.o \
	ds_ctrl.o \
	ds_image.o \
	dsm_ctrl.o \
	twain32_main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
