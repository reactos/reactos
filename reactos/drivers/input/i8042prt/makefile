# $Id$

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = i8042prt

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

TARGET_OBJECTS = $(TARGET_NAME).o keyboard.o registry.o mouse.o ps2pp.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
