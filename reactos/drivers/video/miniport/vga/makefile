# $Id: makefile,v 1.2 2004/03/04 18:49:58 navaraf Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = vgamp

TARGET_DDKLIBS = videoprt.a

TARGET_CFLAGS = -Werror -Wall

TARGET_OBJECTS = \
  initvga.o \
  vgamp.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
