# $Id: Makefile,v 1.14 2002/09/07 15:12:05 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = ndis

TARGET_CFLAGS = -I./include -D_NDIS_ -DBINARY_COMPATIBLE=1

TARGET_OBJECTS = \
  ndis/main.o \
  ndis/40gone.o \
  ndis/50gone.o \
  ndis/buffer.o \
  ndis/cl.o \
  ndis/cm.o \
  ndis/co.o \
  ndis/control.o \
  ndis/hardware.o \
  ndis/io.o \
  ndis/memory.o \
  ndis/miniport.o \
  ndis/protocol.o \
  ndis/string.o \
  ndis/stubs.o \
  ndis/time.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
