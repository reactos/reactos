# $Id: Makefile,v 1.1 2004/04/09 18:27:11 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = vmwinst

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DUNICODE -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501

TARGET_SDKLIBS = kernel32.a ntdll.a

TARGET_GCCLIBS = msvcrt advapi32 kernel32 comctl32 comdlg32 shell32

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
