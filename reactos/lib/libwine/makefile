# $Id: makefile,v 1.2 2004/01/10 00:39:52 sedwards Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = library

TARGET_NAME = libwine

TARGET_CFLAGS = -D__REACTOS__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles

TARGET_OBJECTS = \
	debug.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
