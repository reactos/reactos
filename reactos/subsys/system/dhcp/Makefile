#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE =  windows

TARGET_NAME = dhcp

TARGET_CFLAGS = -D__REACTOS__ -D_WIN32_WINNT=0x0501 -D__USE_W32API -Iinclude

TARGET_OBJECTS = adapter.o alloc.o api.o compat.o dhclient.o dispatch.o \
		hash.o options.o pipe.o privsep.o socket.o tables.o timer.o \
		util.o

TARGET_SDKLIBS = iphlpapi.a ws2_32.a ntdll.a

TARGET_RC_SRCS = dhcp.rc

TARGET_RC_BINSRC = 

TARGET_RC_BINARIES = 

default: all
DEP_OBJECTS = $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
