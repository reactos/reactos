# $Id: Makefile,v 1.1 2003/07/11 18:13:57 chorns Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = dynlink

TARGET_NAME = kmrtint

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = -I../shared -I../kmregtests

TARGET_BASE = 0x50000000

TARGET_OBJECTS = \
  kmrtint.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
