# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = ntvdm

TARGET_INSTALLDIR = system32

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror -D__USE_W32API

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a gdi32.a advapi32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
