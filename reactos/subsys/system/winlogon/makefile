# $Id: makefile,v 1.10.10.1 2004/07/12 19:54:46 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winlogon

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a advapi32.a secur32.a userenv.a

TARGET_OBJECTS = setup.o winlogon.o wlx.o sas.o

TARGET_CFLAGS = -Wall -Werror -DUNICODE -D_UNICODE -I./ -I../../../include

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
