# $Id: Makefile,v 1.24 2004/12/25 11:18:40 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = ndis

TARGET_PCH = include/ndissys.h

TARGET_CFLAGS = -I./include -D__USE_W32API -Wall

TARGET_CFLAGS += -DNDIS_WRAPPER -DNDIS50 -DNDIS50_MINIPORT -DBINARY_COMPATIBLE

TARGET_OBJECTS = \
  ndis/main.o \
  ndis/40gone.o \
  ndis/50gone.o \
  ndis/buffer.o \
  ndis/cl.o \
  ndis/cm.o \
  ndis/co.o \
  ndis/config.o \
  ndis/control.o \
  ndis/hardware.o \
  ndis/io.o \
  ndis/memory.o \
  ndis/miniport.o \
  ndis/protocol.o \
  ndis/string.o \
  ndis/stubs.o \
  ndis/time.o \
  ndis/efilter.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
