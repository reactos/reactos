# $Id: makefile 12852 2005-01-06 13:58:04Z mf $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = dhcpcapi

TARGET_CFLAGS = -Wall -Werror \
                -D__USE_W32API \
                -D_WIN32_IE=0x0500 \
                -D_WIN32_WINNT=0x501 \
                -DWINVER=0x600 \

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = dhcpcapi.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
