#
# $Id: makefile,v 1.0

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = sc

TARGET_CFLAGS = -DDBG

TARGET_SDKLIBS = kernel32.a ntdll.a advapi32.a

TARGET_OBJECTS = command.o config.o main.o query.o setup.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
