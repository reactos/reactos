# $Id: makefile,v 1.12 2004/07/29 04:09:05 arty Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = afd

TARGET_DDKLIBS = \
	$(PATH_TO_TOP)/dk/w32/lib/pseh.a

TARGET_CFLAGS = -I./include -I$(PATH_TO_TOP)/w32api/include/ddk -I$(PATH_TO_TOP)/include/afd -DDBG -D__USE_W32API -Werror -Wall

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
