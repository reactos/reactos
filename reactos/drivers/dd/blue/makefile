# $Id: makefile,v 1.22 2004/01/11 17:31:14 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = blue

TARGET_OBJECTS = blue.o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk
