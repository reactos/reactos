# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = services

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a

TARGET_OBJECTS = $(TARGET_NAME).o database.o

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
