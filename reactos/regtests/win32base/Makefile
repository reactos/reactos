# $Id: Makefile,v 1.1 2003/07/11 18:13:57 chorns Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = dynlink

TARGET_NAME = win32base

TARGET_SDKLIBS = rtshared.a ntdll.a kernel32.a

TARGET_CFLAGS = -I../shared

TARGET_BASE = 0x60000000

TARGET_GENREGTESTS = yes

TARGET_OBJECTS = \
  _regtests.o \
  driver.o \
  file-1.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
