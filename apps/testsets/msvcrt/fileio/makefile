#
# $Id: makefile,v 1.0

PATH_TO_TOP = ../../../..

TEST_ROOT = $(PATH_TO_TOP)/apps/testsets/test

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = fileio

#TARGET_CFLAGS = -I$(TEST_ROOT) -DDBG -DUNICODE -D_UNICODE
TARGET_CFLAGS = -I$(TEST_ROOT) -DDBG

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o wfileio.o main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
