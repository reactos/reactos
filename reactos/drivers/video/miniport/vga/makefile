# $Id: makefile,v 1.3 2004/03/22 16:00:38 navaraf Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = vgamp

TARGET_DDKLIBS = videoprt.a

TARGET_CFLAGS = -Werror -Wall -D__USE_W32API

TARGET_OBJECTS = \
  initvga.o \
  vgamp.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
