# $Id: makefile,v 1.9 2002/09/08 10:22:05 chorns Exp $

# $Id: makefile,v 1.9 2002/09/08 10:22:05 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = ramdrv

TARGET_OBJECTS = ramdrv.o

TARGET_DDKLIBS = unbzip2.a

TARGET_NORC = yes

TARGET_CFLAGS=-I../../../ntoskrnl/include -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
