# $Id: Makefile,v 1.1 2004/02/22 09:59:17 chorns Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = library

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC)

-include Makefile.tests

TARGET_OBJECTS = \
  _regtests.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
