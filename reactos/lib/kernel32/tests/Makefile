# $Id: Makefile,v 1.3 2004/10/20 20:51:21 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC)

TARGET_LIBS = ../kernel32.a

-include Makefile.tests

TARGET_OBJECTS = \
  _rtstub.o \
  _regtests.o \
  _hooks.o \
  _stubs.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
