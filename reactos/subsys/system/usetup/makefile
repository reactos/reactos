# $Id: makefile,v 1.11 2003/04/28 19:44:13 chorns Exp $

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
                 filesup.o format.o infcache.o inicache.o partlist.o progress.o \
                 registry.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF