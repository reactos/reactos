PATH_TO_TOP = ../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = -I../include -I$(REGTESTS_PATH_INC)

TARGET_LIBS = \
  ../ntoskrnl.a \
	$(SDK_PATH_LIB)/rtl.a \
	$(SDK_PATH_LIB)/rosrtl.a \
	$(SDK_PATH_LIB)/pseh.a

-include Makefile.tests

TARGET_OBJECTS = \
  setup.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
