# $Id: makefile,v 1.1 2004/01/10 14:39:21 navaraf Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = vgamp

TARGET_DDKLIBS = videoprt.a

TARGET_CFLAGS = -Werror -Wall

TARGET_OBJECTS = \
  initvga.o \
  vgamp.o \
  vgavideo.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
