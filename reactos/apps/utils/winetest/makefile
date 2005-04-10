PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winetest

TARGET_SDKLIBS = comctl32.a comdlg32.a ws2_32.a

TARGET_OBJECTS = \
	main.o \
	send.o \
	util.o \
	gui.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
