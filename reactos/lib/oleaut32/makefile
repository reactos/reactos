# $Id: makefile,v 1.3 2001/08/21 20:13:07 chorns Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = oleaut32

TARGET_BASE = 0x76260000

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = dllmain.o stubs.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
