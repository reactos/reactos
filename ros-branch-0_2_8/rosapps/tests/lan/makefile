#
# $Id: makefile,v 1.1 2004/10/21 04:58:58 sedwards Exp $

PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = lantest

TARGET_SDKLIBS = ws2_32.a ntdll.a

TARGET_OBJECTS = lantest.o -lstdc++

TARGET_CPPFLAGS = -I$(PATH_TO_TOP)/include -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
