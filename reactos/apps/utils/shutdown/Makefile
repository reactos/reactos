# $Id$

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_INSTALLDIR = system32

TARGET_NAME = shutdown

TARGET_SDKLIBS = kernel32.a user32.a

TARGET_OBJECTS = shutdown.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -D_WIN32_WINNT=0x0501

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
