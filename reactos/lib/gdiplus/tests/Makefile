PATH_TO_TOP = ../../..

TARGET_TYPE = library

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

-include Makefile.tests

TARGET_OBJECTS = \
  _rtstub.o \
  _regtests.o \
  _hooks.o \
  _stubs.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

LIBS = ../gdiplus.a

run: all
	@$(CC) -o _runtest.exe _rtstub.o regtests.a $(SDK_PATH_LIB)/rtshared.a $(LIBS) -lntdll
	@_runtest.exe
	@$(RM) _runtest.exe
