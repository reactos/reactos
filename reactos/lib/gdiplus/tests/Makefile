PATH_TO_TOP = ../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_LIBPATH = .

TARGET_CFLAGS = \
 -I../include \
 -DUNICODE \
 -Wall \
 -Werror \
 -fno-builtin \
 -D__USE_W32API \
 -DWINVER=0x0600 \
 -D_WIN32_WINNT=0x0501 \
 -I$(REGTESTS_PATH_INC)

TARGET_LIBS = ../gdiplus.a

-include Makefile.tests

TARGET_OBJECTS = \
  setup.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
