# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = iphlpapi

TARGET_BASE = 0x777c0000

TARGET_CFLAGS += -DYDEBUG -DUNICODE -D_UNICODE -D__USE_W32API -D__REACTOS__ -D_WIN32_WINNT=0x0500 -Wall 
#-Werror

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = iphlpapi_main.o icmp.o iphlpv6.o media.o registry.o \
	ifenum_reactos.o ipstats_reactos.o resinfo_reactos.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

