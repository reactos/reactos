# $Id: makefile,v 1.1 2002/03/23 19:23:28 robertk Exp $

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

TARGET_DEFONLY=yes

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


MISC_OBJECTS = misc/doscalls.o
#SYNC_OBJECTS =


OBJECTS = $(MISC_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

