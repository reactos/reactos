# $Id: makefile,v 1.10 2003/12/17 23:34:35 vizzini Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = tditest

TARGET_CFLAGS = -I./include -Wall -Werror -DDBG=1

TARGET_OBJECTS = \
  tditest/tditest.o

TARGET_CLEAN = \
  tditest/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
