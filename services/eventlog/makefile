# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = eventlog

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a 

TARGET_OBJECTS = $(TARGET_NAME).o logport.o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
