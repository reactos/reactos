# $Id: makefile,v 1.9 2003/03/13 09:51:11 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = usetup

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

TARGET_OBJECTS = $(TARGET_NAME).o bootsup.o console.o drivesup.o filequeue.o \
                 filesup.o infcache.o inicache.o partlist.o progress.o registry.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF