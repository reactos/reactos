# $Id: makefile,v 1.11 2003/11/14 17:13:25 weiden Exp $

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
