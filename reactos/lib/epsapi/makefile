# $Id: makefile,v 1.3 2003/09/12 17:51:46 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = epsapi

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Werror -Wall

TARGET_OBJECTS = \
 enum/drivers.o \
 enum/modules.o \
 enum/processes.o 
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
