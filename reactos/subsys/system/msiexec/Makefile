#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE =  windows

TARGET_NAME = msiexec

TARGET_CFLAGS = -I$(PATH_TO_TOP)/include/wine -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API

TARGET_OBJECTS = msiexec.o
TARGET_SDKLIBS = msi.a ole32.a ntdll.a user32.a kernel32.a wine.a wine_uuid.a
TARGET_RC_SRCS = version.rc

TARGET_RC_BINSRC = 

TARGET_RC_BINARIES = 

default: all
DEP_OBJECTS = $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
