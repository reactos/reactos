#
# $Id: makefile,v 1.0

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = loadlib

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DUNICODE -D_UNICODE

TARGET_SDKLIBS = kernel32.a ntdll.a 

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
