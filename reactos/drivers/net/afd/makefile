# $Id: makefile,v 1.10.2.1 2004/05/11 03:22:22 arty Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_CFLAGS = -I./include -DDBG -D__USE_W32API -Werror -Wall

TARGET_DDKLIBS = ndis.a $(PATH_TO_TOP)/dk/w32/lib/ntdll.a

TARGET_OBJECTS = \
  afd/afd.o \
  afd/dispatch.o \
  afd/event.o \
  afd/opnclose.o \
  afd/rdwr.o \
  afd/routines.o \
  afd/tdi.o \
  afd/tdiconn.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
