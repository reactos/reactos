# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = adns

TARGET_CFLAGS = \
 -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
 -Wstrict-prototypes -Wmissing-prototypes -Wall -DADNS_JGAA_WIN32

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -Isrc -Iadns_win32

TARGET_OBJECTS = adns_win32/adns_unix_calls.o \
 src/check.o src/event.o src/general.o src/parse.o src/poll.o \
 src/query.o src/reply.o src/setup.o src/transmit.o src/types.o	
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
