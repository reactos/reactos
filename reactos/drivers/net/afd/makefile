# $Id: makefile,v 1.10 2004/03/02 18:18:13 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_CFLAGS = -I./include -DDBG -D__USE_W32API -Werror -Wall

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
