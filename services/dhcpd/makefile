# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = dhcpd

TARGET_CFLAGS = \
 -Wall

TARGET_CFLAGS += -Isrc/include

TARGET_SDKLIBS = ws2_32.a

TARGET_OBJECTS = \
	src/main.o \
	src/display/display.o \
	src/leases/leases.o \
	src/lock/locker.o \
	src/parsing/parser.o \
	src/utils/iputils.o \
	src/utils/macutils.o

#	src/utils/wutils.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
