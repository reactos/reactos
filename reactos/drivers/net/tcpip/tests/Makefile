# $Id: Makefile,v 1.3 2004/10/18 19:11:07 chorns Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = library

TARGET_NAME = regtests

TARGET_PCH =

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC)

-include Makefile.tests

TARGET_OBJECTS = \
  _rtstub.o \
  _regtests.o \
  _hooks.o \
  _stubs.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

run: all
	@$(CC) -o _runtest.exe _rtstub.o regtests.a $(SDK_PATH_LIB)/rtshared.a $(LIBS) -lntdll
	@_runtest.exe
	@$(RM) _runtest.exe
