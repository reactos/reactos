# $Id: Makefile,v 1.2 2003/11/05 22:31:50 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = videoprt

TARGET_CFLAGS += -Wall -Werror

TARGET_OBJECTS = videoprt.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
