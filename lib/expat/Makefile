# $Id: Makefile 13593 2005-02-16 14:01:34Z weiden $

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/config

TARGET_NAME = expat

TARGET_TYPE = library

TARGET_CFLAGS = -D__USE_W32API -DHAVE_EXPAT_CONFIG_H -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -Ilib
TARGET_LFLAGS = -nostartfiles

LIB_OBJECTS = \
	lib/xmlparse.o \
	lib/xmlrole.o \
	lib/xmltok.o

TARGET_OBJECTS = $(LIB_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
