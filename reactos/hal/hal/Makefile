# $Id: Makefile,v 1.2 2002/09/07 15:12:10 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = driver_library

TARGET_ASFLAGS = -D__NTHAL__ -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_CFLAGS = -D__NTHAL__ -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_NAME = hal

HEADER_FILES = $(PATH_TO_TOP)/include/roscfg.h

TARGET_OBJECTS := $(TARGET_NAME).o $(HEADER_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/config.mk

# EOF
