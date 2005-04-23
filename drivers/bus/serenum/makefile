# $Id: makefile 12852 2005-01-06 13:58:04Z mf $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = serenum

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

TARGET_OBJECTS = \
	detect.o      \
	fdo.o         \
	misc.o        \
	pdo.o         \
	serenum.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
