# $Id: Makefile,v 1.5 2003/08/24 16:15:31 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = vfatlib

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = \
 fat12.o \
 fat16.o \
 fat32.o \
 vfatlib.o
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
