#
#  ROS Internet Web Browser
#
#  Makefile
#

PATH_TO_TOP := ../../..

TARGET_TYPE := program

TARGET_APPTYPE := windows

TARGET_NAME := ibrowser

TARGET_INSTALLDIR := system32

TARGET_CFLAGS := \
	-D__USE_W32API -DWIN32 -D_ROS_ \
	-D_WIN32_IE=0x0600 -D_WIN32_WINNT=0x0501 -DWINVER=0x0500 \
	-DUNICODE -fexceptions -Wall -Wno-unused-value \
	-I../../../include/expat

TARGET_CPPFLAGS := $(TARGET_CFLAGS)

TARGET_RCFLAGS := -D__USE_W32API -DWIN32 -D_ROS_ -D__WINDRES__

TARGET_SDKLIBS := gdi32.a comctl32.a ole32.a oleaut32.a shell32.a expat.a

TARGET_GCCLIBS := stdc++ uuid

TARGET_OBJECTS := \
	ibrowser.o \
	mainframe.o \
	webchild.o \
	favorites.o \
	utility/utility.o \
	utility/window.o \
	utility/xmlstorage.o

TARGET_CPPAPP := yes

TARGET_PCH := precomp.h

DEP_OBJECTS := $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk
