# $Id: Makefile,v 1.4 2004/04/13 20:29:29 jfilby Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = driver

TARGET_NAME = kmregtests

TARGET_LIBS = $(SDK_PATH_LIB)/rtshared.a

TARGET_CFLAGS = -I../shared -Werror -Wall
TARGET_CFLAGS += -D__USE_W32API

include Makefile.tests

TARGET_OBJECTS = \
  _regtests.o \
  driver.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
