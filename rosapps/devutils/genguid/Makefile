# $Id: Makefile,v 1.1 2003/08/22 07:26:41 sedwards Exp $

PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = genguid

TARGET_CFLAGS = -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API 

TARGET_SDKLIBS = kernel32.a

TARGET_GCCLIBS = ole32 uuid

TARGET_OBJECTS = genguid.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
