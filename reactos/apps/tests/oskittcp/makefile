#
# $Id: makefile,v 1.2 2004/09/02 02:33:11 arty Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = oskittcp

TARGET_SDKLIBS = ws2_32.a oskittcp.a ntdll.a

TARGET_OBJECTS = tcptest.o -lstdc++

TARGET_CPPFLAGS =  -I$(PATH_TO_TOP)/drivers/lib/oskittcp/include -I$(PATH_TO_TOP)/w32api/include -I$(PATH_TO_TOP)/include -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
