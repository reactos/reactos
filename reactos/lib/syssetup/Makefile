# $Id: Makefile,v 1.11 2004/08/28 11:08:50 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = syssetup

TARGET_BASE = $(TARGET_BASE_LIB_SYSSETUP)

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a gdi32.a user32.a samlib.a userenv.a \
                 comctl32.a setupapi.a

TARGET_CFLAGS = -Wall -Werror -fno-builtin

TARGET_CFLAGS += -D__USE_W32API -D_WIN32_IE=0x0500
TARGET_RCFLAGS += -D__USE_W32API -D_WIN32_IE=0x0500

TARGET_LFLAGS = -nostartfiles

TARGET_OBJECTS = dllmain.o install.o logfile.o wizard.o

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

# EOF
