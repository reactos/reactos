# $Id: makefile,v 1.15 2004/12/03 23:37:39 blight Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_PCH = include/afd.h

TARGET_LIBS = \
	$(PATH_TO_TOP)/dk/w32/lib/libpseh.a

TARGET_CFLAGS = -I./include -I$(PATH_TO_TOP)/w32api/include/ddk -I$(PATH_TO_TOP)/include/afd -D__USE_W32API -Werror -Wall

TARGET_OBJECTS = \
	afd/bind.o \
	afd/connect.o \
	afd/context.o \
	afd/info.o \
	afd/listen.o \
	afd/lock.o \
	afd/main.o \
	afd/read.o \
	afd/select.o \
	afd/tdi.o \
	afd/tdiconn.o \
	afd/write.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
