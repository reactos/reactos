# $Id$

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_SDKLIBS = ntdll.a fmifs.a

TARGET_NAME = format

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -Werror -Wall -Wno-format

TARGET_OBJECTS = \
	$(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
