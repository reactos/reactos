# $Id: makefile,v 1.1 2002/03/23 19:23:28 robertk Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = os2ss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

#OBJECTS_API = 

OBJECTS_MISC = \
  $(TARGET_NAME).o 

TARGET_OBJECTS = \
  $(OBJECTS_API) 
#  \
#  $(OBJECTS_MISC)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
