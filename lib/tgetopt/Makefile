# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = tgetopt

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = getopt.o _wgetopt.o
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
