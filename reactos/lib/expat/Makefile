# $Id: Makefile,v 1.3 2004/05/29 21:24:43 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_NAME = libexpat

TARGET_TYPE = dynlink
TARGET_BASE = $(TARGET_BASE_LIB_EXPAT)
TARGET_NORC = yes
TARGET_DEFONLY = yes
TARGET_DEFNAME = lib/libexpat

TARGET_CFLAGS = -D__USE_W32API -DHAVE_EXPAT_CONFIG_H -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions
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
