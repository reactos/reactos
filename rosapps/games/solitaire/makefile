
PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = windows
# change to console for debugging purposes...
#TARGET_APPTYPE = console

TARGET_INSTALLDIR = system32

TARGET_NAME = sol

TARGET_SDKLIBS = kernel32.a user32.a gdi32.a cards.a comctl32.a

TARGET_GCCLIBS = stdc++

TARGET_OBJECTS = $(SOL_OBJECTS) $(CARDLIB_OBJECTS)

SOL_OBJECTS = \
	solcreate.o \
	solgame.o \
	solitaire.o

CARDLIB_OBJECTS = \
	cardlib/cardbitmaps.o \
	cardlib/cardbutton.o \
	cardlib/cardcolor.o \
	cardlib/cardcount.o \
	cardlib/cardlib.o \
	cardlib/cardregion.o \
	cardlib/cardrgndraw.o \
	cardlib/cardrgnmouse.o \
	cardlib/cardstack.o \
	cardlib/cardwindow.o \
	cardlib/dropzone.o

TARGET_CPPFLAGS = -Icardlib -Wall -D__USE_W32API -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__REACTOS__

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
