# $Id: makefile,v 1.9 2003/11/20 06:31:51 vizzini Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = tditest

TARGET_CFLAGS = -I./include -Wall -Werror

TARGET_OBJECTS = \
  tditest/tditest.o

TARGET_CLEAN = \
  tditest/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
