# $Id: Makefile,v 1.3 2003/12/07 11:34:41 chorns Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = driver

TARGET_NAME = kmregtests

TARGET_LIBS = $(SDK_PATH_LIB)/rtshared.a

TARGET_CFLAGS = -I../shared -Werror -Wall

include Makefile.tests

TARGET_OBJECTS = \
  _regtests.o \
  driver.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
