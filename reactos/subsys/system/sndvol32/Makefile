# $Id: Makefile,v 1.1 2004/12/29 22:37:13 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = sndvol32

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API -DUNICODE -D_UNICODE -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501

TARGET_SDKLIBS = rosrtl.a user32.a kernel32.a ntdll.a

TARGET_GCCLIBS = winmm msvcrt advapi32 kernel32 comctl32

TARGET_OBJECTS = $(TARGET_NAME).o mixer.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
