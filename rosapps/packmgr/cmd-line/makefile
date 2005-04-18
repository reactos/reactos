
TARGET_NAME = ros-get
TARGET_TYPE = program
TARGET_APPTYPE = console
PATH_TO_TOP = ../../../reactos
TARGET_INSTALLDIR = bin

TARGET_SDKLIBS = package.a

TARGET_GCCLIBS = stdc++ uuid

TARGET_OBJECTS = main.o

TARGET_CFLAGS = \
	-D__USE_W32API -DWIN32 -D_ROS_ \
	-D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 \
	-DUNICODE -Wall -I../lib

TARGET_CPPFLAGS = $(TARGET_CFLAGS)

TARGET_RCFLAGS =  -D__USE_W32API -DNDEBUG -DWIN32 -D_ROS_ -D__WINDRES__ -DUNICODE

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk
