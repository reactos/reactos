# $Id: makefile,v 1.3 2002/05/30 15:11:46 robertk Exp $

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

DOSCALLS_MISC_OBJECTS = misc/doscalls.o file/openclose.o run/process.o devices/devices.o

#SYNC_OBJECTS =

TARGET_OBJECTS = $(DOSCALLS_MISC_OBJECTS)


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


# EOF
