# $Id: makefile,v 1.3 2004/01/11 20:46:05 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = tcpsvr

TARGET_SDKLIBS = kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
