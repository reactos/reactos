# $Id: Makefile,v 1.2 2003/11/19 05:53:37 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = regtests

TARGET_SDKLIBS = rtshared.a ntdll.a kernel32.a

TARGET_CFLAGS = -I../shared -Wall -Werror

TARGET_OBJECTS = \
  regtests.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
