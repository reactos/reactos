
PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = nameserverlist

TARGET_SDKLIBS = iphlpapi.a ws2_32.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror -g

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
