# $Id: makefile,v 1.7 2004/01/11 20:46:05 navaraf Exp $

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
  afd/tdi.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
