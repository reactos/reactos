# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = winmm

TARGET_BASE = 0x777c0000

TARGET_CFLAGS = 

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = \
	dllmain.o \
	misc\stubs.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk