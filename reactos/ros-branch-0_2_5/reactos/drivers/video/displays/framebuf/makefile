# $Id: makefile,v 1.1 2004/01/13 17:18:33 navaraf Exp $

PATH_TO_TOP = ../../../..

TARGET_BASE = 0x70000000

TARGET_TYPE = gdi_driver

TARGET_NAME = framebuf

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API 

TARGET_OBJECTS = \
	enable.o \
	palette.o \
	pointer.o \
	screen.o \
	surface.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS = $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF
