#
# $Id: makefile,v 1.0

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = regdump

#TARGET_CFLAGS = -DWIN32_REGDBG -DUNICODE -D_UNICODE
TARGET_CFLAGS = -DWIN32_REGDBG

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = $(TARGET_NAME).o regcmds.o regproc.o main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
