# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_SDKLIBS = kernel32.a ntdll.a fmifs.a

TARGET_NAME = format

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API -Werror -Wall -Wno-format

TARGET_OBJECTS = \
	$(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
