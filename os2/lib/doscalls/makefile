# $Id: makefile,v 1.2 2002/03/24 18:52:10 ea Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = dynlink

TARGET_NAME = doscalls

TARGET_BASE = 0x30000000

TARGET_CFLAGS = -DDOSCALLS_BASE=$(TARGET_DLLBASE)

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = ntdll.a

TARGET_GCCLIBS = gcc

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = file/*.o vio/*.o sync/*.o mem/*.o \
				misc/*.o mou/*.o kbd/*.o run/*.o

DOSCALLS_MISC_OBJECTS = misc/doscalls.o

#SYNC_OBJECTS =

TARGET_OBJECTS = $(DOSCALLS_MISC_OBJECTS)


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


# EOF
