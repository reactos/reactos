# $Id: makefile,v 1.10 2003/11/16 22:39:35 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = sermouse

TARGET_OBJECTS = \
  sermouse.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API 

#mouse.o - Old Serial mouse driver

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
