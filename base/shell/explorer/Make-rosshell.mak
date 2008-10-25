#
#  ReactOS shell
#
#  Makefile
#

PATH_TO_TOP := ../../..

TARGET_TYPE := program

TARGET_APPTYPE := windows

TARGET_NAME := rosshell

TARGET_INSTALLDIR := .

TARGET_CFLAGS := \
	-D__USE_W32API -DWIN32 -D_ROS_ \
	-D_WIN32_IE=0x0600 -D_WIN32_WINNT=0x0501 -DWINVER=0x0500 \
	-DUNICODE -fexceptions -Wall -g

TARGET_CPPFLAGS := $(TARGET_CFLAGS)

TARGET_RCFLAGS := -D__USE_W32API -DWIN32 -D_ROS_ -D__WINDRES__

TARGET_SDKLIBS := \
	gdi32.a user32.a comctl32.a ole32.a oleaut32.a shell32.a \
	notifyhook.a ws2_32.a msimg32.a

TARGET_GCCLIBS := stdc++ uuid

TARGET_OBJECTS := \
	explorer.o \
	i386-stub-win32.o \
	desktop/desktop.o \
	dialogs/searchprogram.o \
	dialogs/settings.o \
	shell/entries.o \
	shell/shellfs.o \
	shell/pane.o \
	shell/winfs.o \
	services/startup.o \
	services/shellservices.o \
	taskbar/desktopbar.o \
	taskbar/taskbar.o \
	taskbar/startmenu.o \
	taskbar/traynotify.o \
	taskbar/quicklaunch.o \
	taskbar/favorites.o \
	utility/shellclasses.o \
	utility/utility.o \
	utility/window.o \
	utility/dragdropimpl.o \
	utility/shellbrowserimpl.o \
	utility/xmlstorage.o \
	utility/xs-native.o

TARGET_CPPAPP := yes

TARGET_PCH := precomp.h

SUBDIRS := notifyhook

DEP_OBJECTS := $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk
