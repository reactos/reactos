# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_NAME = undis

TARGET_PCH = include/ndissys.h

TARGET_CFLAGS = -Iinclude -I../../net/ndis/include -DNDIS_WRAPPER -Wall -Werror 

TARGET_SDKLIBS = ntdll.a

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
