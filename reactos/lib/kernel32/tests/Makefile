# $Id: Makefile,v 1.1.18.1 2004/10/25 14:48:32 ion Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC)

TARGET_LIBS = ../kernel32.a

-include Makefile.tests

TARGET_OBJECTS = \
  setup.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
