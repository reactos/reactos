# $Id: makefile,v 1.9 2004/03/20 15:58:16 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winlogon

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a advapi32.a userenv.a secur32.a

TARGET_OBJECTS = setup.o winlogon.o wlx.o

TARGET_CFLAGS = -Wall -Werror -DUNICODE -I./ -I../../../include

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
