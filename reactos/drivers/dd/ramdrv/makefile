# $Id: makefile,v 1.8 2002/09/07 15:11:59 chorns Exp $

# $Id: makefile,v 1.8 2002/09/07 15:11:59 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = ramdrv

TARGET_OBJECTS = ramdrv.o

TARGET_DDKLIBS = unbzip2.a

TARGET_CFLAGS=-I$(PATH_TO_TOP)/ntoskrnl/include -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
