# $Id: makefile,v 1.1 2001/11/28 23:35:16 ea Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_PATH = misc

TARGET_NAME = psapi

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = \
	misc/dllmain.o \
	misc/stubs.o \
	$(TARGET_NAME).coff
	
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

TARGET_CLEAN = *.o *.coff misc/*.o

# EOF
