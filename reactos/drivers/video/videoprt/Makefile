# $Id: Makefile,v 1.1 2004/01/19 15:56:53 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = videoprt

TARGET_CFLAGS += -Wall -Werror -D__USE_W32API -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_OBJECTS = videoprt.o int10.o services.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
