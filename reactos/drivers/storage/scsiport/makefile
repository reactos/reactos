# $Id: makefile,v 1.8 2004/06/10 08:20:20 hbirr Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = export_driver

TARGET_NAME = scsiport

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Werror -Wall

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
