PATH_TO_TOP = ../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC) \
                -D__USE_W32API \
                -D_WIN32K_

TARGET_LIBS = ../win32k.a

-include Makefile.tests

TARGET_OBJECTS = \
  _rtstub.o \
  _regtests.o \
  _hooks.o \
  _stubs.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
