#
#  ReactOS winefile
#
#  Makefile
#

PATH_TO_TOP = ../../..

TARGET_NAME = winefile
TARGET_TYPE = program
TARGET_APPTYPE = windows
TARGET_INSTALLDIR := .

TARGET_CFLAGS := \
	-D__USE_W32API -DWIN32 -D_ROS_ \
	-D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 \
	-DUNICODE -Wall

TARGET_CPPFLAGS := $(TARGET_CFLAGS)

TARGET_RCFLAGS := -D__USE_W32API -DNDEBUG -DWIN32 -D_ROS_ -D__WINDRES__ -DUNICODE

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	comctl32.a \
	comdlg32.a \
	ole32.a \
	oleaut32.a \
	shell32.a \
	mpr.a

TARGET_GCCLIBS := uuid

TARGET_OBJECTS = \
	license.o \
	winefile.o

DEP_OBJECTS := $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk
