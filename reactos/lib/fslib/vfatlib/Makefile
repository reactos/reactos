# $Id: Makefile,v 1.2 2003/08/07 04:03:22 royce Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = vfatlib

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = \
 vfatlib.o
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
