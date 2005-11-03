# $Id: Makefile,v 1.2 2003/01/07 16:23:10 robd Exp $
#
#

PATH_TO_TOP = ../../../reactos

PATH_TO_OS2_TOP = ../..

TARGET_TYPE = program

TARGET_NORC = yes

#TARGET_APPTYPE = console
TARGET_APPTYPE = windows

TARGET_NAME = bepslep

TARGET_CFLAGS = -D__SUBSYSTEM_WINDOWS__

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = doscalls.a

TARGET_GCCLIBS = stdc++

#TARGET_LIBS = $(PATH_TO_OS2_TOP)/lib/crt0w32.o \
#	      $(PATH_TO_TOP)/dk/psx/lib/psxdll.a

#	$(PATH_TO_OS2_TOP)/dll/doscalls.dll 

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
