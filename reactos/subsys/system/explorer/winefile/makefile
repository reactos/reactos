#
#  ReactOS winfile explorer
#
#  Makefile
#

PATH_TO_TOP = ../../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = explorer

TARGET_CFLAGS = -fexceptions -O2 -DNDEBUG -DWIN32 -D_ROS_ -W

TARGET_RCFLAGS = -DNDEBUG -DWIN32 -D_ROS_

ifdef UNICODE
TARGET_CFLAGS += -DUNICODE
TARGET_CPPFLAGS += -DUNICODE
MK_DEFENTRY := _wWinMain@16
endif

VPATH += ../utility
VPATH += ../shell
VPATH += ../taskbar

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_GCCLIBS = comctl32 ole32 uuid

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	advapi32.a \
	version.a

TARGET_OBJECTS = \
	desktop.o \
	ex_bar.o \
	ex_clock.o \
	ex_menu.o \
	ex_shutdwn.o \
	license.o \
	startup.o \
	winefile.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
