# $Id: Makefile,v 1.1 2003/12/28 21:36:53 gvg Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = create-links

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_GCCLIBS = ole32 uuid shell32 shlwapi

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -D_WIN32_IE=0x0400

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
