# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = rundll32

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = kernel32.a user32.a

TARGET_OBJECTS = rundll32.o

TARGET_CFLAGS += -DUNICODE -Wall -Werror -D__USE_W32API

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
