# $Id: Makefile,v 1.4 2004/10/23 21:05:11 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC)

TARGET_LIBS = ../kernel32.a

-include Makefile.tests

TARGET_OBJECTS = $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
