# $Id:

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = green

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

TARGET_OBJECTS = \
	$(TARGET_NAME).o \
	createclose.o \
	dispatch.o \
	keyboard.o \
	misc.o \
	pnp.o \
	screen.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
