# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = wine_uuid

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -I$(PATH_TO_TOP)/include/wine

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = uuid.o
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/tools/depend.mk
