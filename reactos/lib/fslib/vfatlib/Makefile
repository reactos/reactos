# $Id: Makefile,v 1.4 2003/08/22 13:53:02 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = vfatlib

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = \
 fat16.o \
 fat32.o \
 vfatlib.o
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
