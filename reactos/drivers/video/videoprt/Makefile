# $Id: Makefile,v 1.2 2004/03/04 18:51:58 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = videoprt

TARGET_CFLAGS += -Wall -Werror -D__USE_W32API -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_OBJECTS = \
	int10.o \
	interrupt.o \
	videoprt.o \
	services.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
