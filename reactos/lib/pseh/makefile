# $Id: makefile,v 1.1 2004/06/02 18:36:55 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = pseh

TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall -D__USE_W32API

include $(PATH_TO_TOP)/config
include makefile.$(ARCH)

TARGET_OBJECTS = framebased.o $(TARGET_ARCH_OBJECTS)
DEP_OBJECTS = $(TARGET_OBJECTS)
 
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk

# EOF
