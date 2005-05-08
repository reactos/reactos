# $Id$

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = partinfo

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror -Wno-format

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
