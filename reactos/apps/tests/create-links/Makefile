# $Id: Makefile,v 1.2 2004/08/18 02:16:00 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = create-links

TARGET_SDKLIBS = kernel32.a gdi32.a ole32.a shell32.a shlwapi.a

TARGET_GCCLIBS = uuid

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -D_WIN32_IE=0x0400

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
