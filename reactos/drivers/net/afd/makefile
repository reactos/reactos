# $Id: makefile,v 1.5 2001/08/21 20:13:15 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_CFLAGS = -I./include -DDBG

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
