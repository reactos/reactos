
PATH_TO_TOP=../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = netreg

TARGET_SDKLIBS = 

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_GCCLIBS = ws2_32 stdc++

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

TARGET_CPPFLAGS = -I$(PATH_TO_TOP)/w32api/include -D__USE_W32API

# EOF
