# $Id: makefile,v 1.8 2003/04/13 03:24:26 hyperion Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = cabman

TARGET_OBJECTS = cabinet.o mszip.o raw.o  main.o dfp.o

TARGET_SDKLIBS = zlib.a

TARGET_CFLAGS = -I$(PATH_TO_TOP)/lib/zlib

TARGET_CPPFLAGS = $(TARGET_CFLAGS)

TARGET_GCCLIBS = stdc++

TARGET_NORC = yes

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

