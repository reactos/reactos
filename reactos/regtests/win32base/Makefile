# $Id: Makefile,v 1.2 2003/11/19 05:53:38 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = dynlink

TARGET_NAME = win32base

TARGET_SDKLIBS = rtshared.a ntdll.a kernel32.a

TARGET_CFLAGS = -I../shared -Wall -Werror

TARGET_BASE = 0x60000000

TARGET_GENREGTESTS = yes

TARGET_OBJECTS = \
  _regtests.o \
  driver.o \
  file-1.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
