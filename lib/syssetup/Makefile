# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = syssetup

TARGET_BASE = $(TARGET_BASE_LIB_SYSSETUP)

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a gdi32.a user32.a samlib.a userenv.a \
                 comctl32.a setupapi.a ole32.a shell32.a shlwapi.a

TARGET_GCCLIBS = uuid

TARGET_CFLAGS = -Wall -Werror -fno-builtin

TARGET_CFLAGS += -D__USE_W32API -D_WIN32_IE=0x0600 -D_WIN32_WINNT=0x0501 -DUNICODE -D_UNICODE
TARGET_RCFLAGS += -D__USE_W32API -D_WIN32_IE=0x0600  -D_WIN32_WINNT=0x0501 -DUNICODE -D_UNICODE

TARGET_LFLAGS = -nostartfiles

TARGET_OBJECTS = dllmain.o install.o logfile.o wizard.o

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(TOOLS_PATH)/depend.mk

# EOF
