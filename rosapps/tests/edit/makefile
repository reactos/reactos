
PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = edittest

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = \
	edittest.o \
	utils.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
