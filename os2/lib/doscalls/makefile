# $Id: makefile,v 1.4 2002/07/26 00:23:12 robertk Exp $

PATH_TO_TOP = ../../../reactos

PATH_TO_OS2_TOP = ../..

TARGET_TYPE = dynlink

# TARGET_DEFONLY = yes

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

TARGET_SDKLIBS = ntdll.a

#TARGET_GCCLIBS = gcc

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = file/*.o vio/*.o sync/*.o mem/*.o \
				misc/*.o mou/*.o kbd/*.o run/*.o

DOSCALLS_MISC_OBJECTS = misc/doscalls.o misc/error.o 

DOSCALLS_OTHER_O = file/openclose.o run/process.o devices/devices.o

#SYNC_OBJECTS =

TARGET_OBJECTS = $(DOSCALLS_OTHER_O) $(DOSCALLS_MISC_OBJECTS)


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


# EOF
