# $Id: makefile,v 1.1.2.1 2004/12/13 09:39:26 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = library

TARGET_NAME = dxguid

TARGET_CFLAGS = -D__REACTOS__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

TARGET_LFLAGS = -Wl

TARGET_OBJECTS = \
	dxguid-mingw.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF