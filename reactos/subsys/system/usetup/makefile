# $Id: makefile,v 1.6 2002/12/06 21:39:03 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = usetup

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

TARGET_OBJECTS = $(TARGET_NAME).o console.o drivesup.o filequeue.o filesup.o \
                 inicache.o partlist.o progress.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF