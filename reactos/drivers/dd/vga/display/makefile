# $Id: makefile,v 1.18 2001/08/21 20:13:12 chorns Exp $

PATH_TO_TOP = ../../../..

TARGET_BASE = 0x70000000

TARGET_TYPE = gdi_driver

TARGET_NAME = vgaddi

MAIN_OBJECTS = \
  main/enable.o

OTHER_OBJECTS = \
  objects/screen.o \
  objects/pointer.o \
  objects/lineto.o \
  objects/paint.o \
  objects/bitblt.o \
  objects/transblt.o

VGAVIDEO_OBJECTS = \
  vgavideo/vgavideo.o  

TARGET_OBJECTS = \
  $(MAIN_OBJECTS) \
  $(OTHER_OBJECTS) \
  $(VGAVIDEO_OBJECTS)

TARGET_CLEAN = \
  main/*.o \
  objects/*.o \
  vgavideo/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
