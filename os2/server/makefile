# $Id: makefile,v 1.3 2003/01/12 02:03:06 robd Exp $

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_NORC = yes

TARGET_APPTYPE = native

TARGET_NAME = os2ss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

TARGET_SDKLIBS = ntdll.a kernel32.a csrss.a

TARGET_GCCLIBS = stdc++

OBJECTS_API = 

OBJECTS_MISC = \
  $(TARGET_NAME).o 

TARGET_OBJECTS = \
  $(OBJECTS_API) \
  $(OBJECTS_MISC)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
