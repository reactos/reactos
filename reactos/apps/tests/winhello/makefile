# $Id: makefile,v 1.5 2003/11/14 17:13:22 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = winhello

TARGET_SDKLIBS = kernel32.a gdi32.a ntdll.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
