# $Id: makefile,v 1.1 2004/09/29 04:54:26 arty Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = undis

TARGET_PCH = include/ndissys.h

TARGET_CFLAGS = -Iinclude -I../../net/ndis/include -DNDIS_WRAPPER -Wall -Werror 

#  ndis/io.o 
#  ndis/time.o
#  ndis/cm.o
#  ndis/miniport.o
#  ndis/cl.o 
#  ndis/co.o 
#  ndis/40gone.o \
#  ndis/50gone.o 
#  ndis/stubs.o
TARGET_OBJECTS = \
  ndis/compat.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
