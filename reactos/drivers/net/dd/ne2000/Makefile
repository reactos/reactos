# $Id: Makefile,v 1.12 2003/11/14 17:13:28 weiden Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = ne2000

TARGET_CFLAGS = -I./include -Werror -Wall

TARGET_OBJECTS = \
  ne2000/8390.o \
  ne2000/main.o

TARGET_DDKLIBS = ndis.a

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
