# $Id: makefile,v 1.4 2003/04/02 22:09:57 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psapi

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = -I./include -Wall

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_BASE = 0x68F70000

TARGET_OBJECTS = \
	misc/dllmain.o \
	misc/malloc.o \
	misc/stubs.o \
	misc/win32.o \
	enum/module.o \
	enum/process.o
	
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

# EOF
