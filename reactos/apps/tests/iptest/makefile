#
# $Id: makefile,v 1.1 2004/10/04 04:26:00 arty Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = iptest

TARGET_SDKLIBS = ws2_32.a ip.a oskittcp.a undis.a ntdll.a 

TARGET_OBJECTS = iptest.o

TARGET_CPPFLAGS =  -I$(PATH_TO_TOP)/drivers/lib/oskittcp/include -I$(PATH_TO_TOP)/w32api/include -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/drivers/net/tcpip/include -g

TARGET_GCCLIBS = stdc++

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
