# $Id: makefile,v 1.6 2003/01/07 16:23:11 robd Exp $

PATH_TO_TOP = ../../../reactos

PATH_TO_OS2_TOP = ../..

TARGET_TYPE = dynlink

#TARGET_DEFONLY = yes

TARGET_NAME = doscalls

TARGET_BASE = 0x60000000

TARGET_CPPFLAGS =\
	-Wall \
	-I$(PATH_TO_OS2_TOP)/include \
	-D__DOSCALLSDLL__
# removed CFLAGX
#	-fno-builtin \
#	-nostdinc \
#	-nostdlib \

TARGET_LFLAGS = -nostartfiles

TARGET_LIBS = doscalls.a

TARGET_SDKLIBS = ntdll.a

TARGET_GCCLIBS = stdc++

TARGET_OBJECTS = $(TARGET_NAME).o


MISC_OBJECTS = misc/doscalls.o misc/error.o 

FILE_OBJECTS = file/openclose.o

RUN_OBJECTS = run/process.o

DEVICES_OBJECTS = devices/devices.o

OTHER_OBJECTS =

SYNC_OBJECTS =

TARGET_OBJECTS = $(FILE_OBJECTS) $(RUN_OBJECTS) $(DEVICES_OBJECTS) \
		 $(MISC_OBJECTS) $(OTHER_OBJECTS) $(SYNC_OBJECTS)

#TARGET_CLEAN = file/*.o vio/*.o sync/*.o mem/*.o \
#		misc/*.o mou/*.o kbd/*.o run/*.o \
#		doscalls.dll temp.exp

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


# EOF
