# $Id: makefile,v 1.9 2004/02/07 04:50:44 arty Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_CFLAGS = -I./include -DDBG -Werror -Wall

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
