# $Id: makefile,v 1.1 2002/06/25 21:10:14 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = eventlog

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a 

TARGET_OBJECTS = $(TARGET_NAME).o logport.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
