# $Id: Makefile,v 1.13 2001/08/21 20:13:16 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = tdi

TARGET_OBJECTS = \
  ne2000/8390.o \
  ne2000/main.o

OBJECTS_CTE = \
  cte/string.o \
  cte/stubs.o

OBJECTS_MISC = \
  misc/main.o

OBJECTS_TDI = \
  tdi/dereg.o \
  tdi/handler.o \
  tdi/obsolete.o \
  tdi/stubs.o
  
TARGET_OBJECTS = \
  $(OBJECTS_CTE) \
  $(OBJECTS_MISC) \
  $(OBJECTS_TDI)

TARGET_CLEAN = \
  cte/*.o \
  misc/*.o \
  misc/*.coff \
  tdi/*.o

TARGET_PATH = misc

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
