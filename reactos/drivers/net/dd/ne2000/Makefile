# $Id$

PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = ne2000

TARGET_CFLAGS = -I./include -D__USE_W32API -Werror -Wall

TARGET_OBJECTS = \
  ne2000/8390.o \
  ne2000/main.o

TARGET_DDKLIBS = ndis.a

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
