# $Id: Makefile,v 1.2 2003/11/13 15:24:14 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = export_driver

TARGET_NAME = diskdump

TARGET_DDKLIBS = 

TARGET_OBJECTS = $(TARGET_NAME).o diskdump_helper.o

TARGET_CFLAGS = -Werror -Wall

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
