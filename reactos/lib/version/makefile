# $Id: makefile,v 1.15 2004/05/29 21:24:47 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = version

TARGET_BASE = $(TARGET_BASE_LIB_VERSION)

TARGET_CFLAGS = -fno-builtin -Wall

# require os code to explicitly request A/W version of structs/functions
DEFINES = \
	-D_DISABLE_TIDENTS \
	-D__USE_W32API \
	-D__REACTOS__ \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 \
	-DWINVER=0x501 
	
TARGET_CFLAGS += \
	$(DEFINES) \
	-I$(PATH_TO_TOP)/include/wine

TARGET_RCFLAGS += -D__REACTOS__	-D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = libwine.a kernel32.a ntdll.a 

TARGET_OBJECTS = \
	misc/libmain.o \
	misc/stubs.o \
	info.o \
	install.o \
	resource.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
