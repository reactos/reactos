# $Id: makefile,v 1.8 2002/09/07 15:12:46 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ws2_32

TARGET_BASE = 0x77400000

TARGET_CFLAGS = -I./include -DUNICODE -DLE -DDBG

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = \
  misc/bsd.o misc/catalog.o misc/dllmain.o \
  misc/event.o misc/handle.o misc/ns.o \
  misc/sndrcv.o misc/stubs.o misc/upcall.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

TARGET_CLEAN = misc/*.o

# EOF
