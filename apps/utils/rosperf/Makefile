PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = rosperf

TARGET_SDKLIBS = version.a gdi32.a user32.a kernel32.a ntdll.a

TARGET_OBJECTS = rosperf.o testlist.o fill.o lines.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/tools/depend.mk

# EOF
