# $Id: Makefile,v 1.3 2003/11/30 19:15:21 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = videoprt

TARGET_CFLAGS += -Wall -Werror -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_OBJECTS = videoprt.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
