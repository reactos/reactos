# $Id: makefile,v 1.10 2004/03/28 12:21:41 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winlogon

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a advapi32.a userenv.a secur32.a

TARGET_OBJECTS = setup.o winlogon.o wlx.o sas.o

TARGET_CFLAGS = -Wall -Werror -DUNICODE -D_UNICODE -I./ -I../../../include

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
