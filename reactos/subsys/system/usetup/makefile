# $Id: makefile,v 1.12 2003/08/09 16:32:25 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_BOOTSTRAP_NAME = smss.exe

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = usetup

TARGET_SDKLIBS = vfatlib.a ntdll.a

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

TARGET_OBJECTS = $(TARGET_NAME).o bootsup.o console.o drivesup.o filequeue.o \
                 filesup.o format.o fslist.o infcache.o inicache.o partlist.o \
                 progress.o registry.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF