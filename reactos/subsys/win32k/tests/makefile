PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I$(REGTESTS_PATH_INC) \
                -D__USE_W32API \
                -D_WIN32K_

-include Makefile.tests

TARGET_OBJECTS = \
  _regtests.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

