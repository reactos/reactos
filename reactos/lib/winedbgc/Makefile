# $Id: Makefile,v 1.5 2003/11/14 17:13:32 weiden Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = winedbgc

#TARGET_CFLAGS = -fno-rtti -D_ROS_ -D__WINE__

#TARGET_CFLAGS = -g -D__NTDLL__
TARGET_CFLAGS = -D__NTDLL__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles

TARGET_BASE = 0x77a90000

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = \
	debug.o \
	libmain.o \
	porting.o \
	trace.o \
	winedbgc.o \
	winedbgc.dll.dbg.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
