# $Id: Makefile,v 1.3 2003/12/07 11:34:41 chorns Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = dynlink

TARGET_NAME = win32base

TARGET_SDKLIBS = rtshared.a ntdll.a kernel32.a

TARGET_CFLAGS = -I../shared -Wall -Werror

TARGET_BASE = 0x60000000

include Makefile.tests

TARGET_OBJECTS = \
  _regtests.o \
  driver.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
