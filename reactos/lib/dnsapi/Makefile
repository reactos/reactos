# $Id: Makefile,v 1.1 2003/12/18 10:54:48 arty Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = dnsapi

TARGET_BASE = 0x78000000

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = adns.a kernel32.a iphlpapi.a ws2_32.a msvcrt.a ntdll.a \
	rosrtl.a

TARGET_GCCLIBS = gcc

TARGET_CFLAGS = -Werror -Wall -I$(PATH_TO_TOP)/lib/adns/src -Iinclude \
	-I$(PATH_TO_TOP)/lib/adns/adns_win32

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -DADNS_JGAA_WIN32

TARGET_OBJECTS = \
	dnsapi/context.o dnsapi/names.o dnsapi/query.o dnsapi/free.o \
	dnsapi/adns.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
