# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = userinit

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DUNICODE -D_UNICODE

TARGET_SDKLIBS = kernel32.a gdi32.a user32.a advapi32.a ntdll.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
