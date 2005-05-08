# $Id$
# 
#  ReactOS Win32 SM Query Tool
# 
PATH_TO_TOP = ../../..
TOOLS_PATH = $(PATH_TO_TOP)/tools

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = sm

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API -DANONYMOUSUNIONS -Wall -Werror

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_SDKLIBS = smdll.a ntdll.a

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
