# $Id: makefile,v 1.3 2003/11/13 14:17:51 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = atapi

TARGET_DDKLIBS = scsiport.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Werror -Wall

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
