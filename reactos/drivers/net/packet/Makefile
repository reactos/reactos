# $Id: Makefile,v 1.1 2002/06/19 15:43:15 robd Exp $

PATH_TO_TOP = ../../..

#TARGET_TYPE = export_driver
TARGET_TYPE = driver

TARGET_NAME = packet

TARGET_CFLAGS = -DNDIS50
# -disable-stdcall-fixup

TARGET_OBJECTS = \
  packet.o \
  read.o \
  openclos.o \
  write.o \
  win_bpf_filter.o

TARGET_DDKLIBS = ndis.a

TARGET_CLEAN = \
  *.o \
  *.coff

TARGET_PATH = .

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
