# $Id: makefile,v 1.3 2004/01/16 04:43:19 mtempel Exp $

PATH_TO_TOP=../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = ping

TARGET_CFLAGS = -D__USE_W32_SOCKETS

TARGET_SDKLIBS = ntdll.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
