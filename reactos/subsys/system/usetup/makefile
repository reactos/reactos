# $Id: makefile,v 1.15 2004/02/23 11:58:27 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_BOOTSTRAP_NAME = smss.exe

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = usetup

TARGET_SDKLIBS = zlib.a vfatlib.a ntdll.a

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__ -I$(PATH_TO_TOP)/lib/zlib -Wall -Werror -Wno-format

TARGET_OBJECTS = bootsup.o cabinet.o console.o drivesup.o \
                 filequeue.o filesup.o format.o fslist.o infcache.o \
                 inicache.o partlist.o progress.o registry.o usetup.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF