# $Id: makefile,v 1.7 2002/02/05 02:30:13 phreak Exp $

# $Id: makefile,v 1.7 2002/02/05 02:30:13 phreak Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = ramdrv

TARGET_OBJECTS = ramdrv.o

TARGET_DDKLIBS = unbzip2.a

TARGET_NORC = yes

TARGET_CFLAGS=-I../../../ntoskrnl/include -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
