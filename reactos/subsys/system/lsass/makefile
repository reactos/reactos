# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = lsass

TARGET_INSTALLDIR = system32

TARGET_CFLAGS += -Wall -Werror -D__USE_W32API

TARGET_OBJECTS = lsass.o

TARGET_SDKLIBS = ntdll.a kernel32.a lsasrv.a

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
