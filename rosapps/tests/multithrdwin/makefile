# $Id: makefile,v 1.1 2004/10/21 04:58:59 sedwards Exp $

PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = multithrdwin

TARGET_SDKLIBS = kernel32.a gdi32.a ntdll.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
