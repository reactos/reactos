# $Id: makefile,v 1.7 2003/12/25 14:06:14 chorns Exp $

PATH_TO_TOP = ../../../..

TARGET_PATH = common

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = roshttpd

TARGET_CPPFLAGS = -I./include -DUNICODE -D_UNICODE -DDBG -Wno-deprecated

TARGET_GCCLIBS = stdc++

TARGET_SDKLIBS = kernel32.a ws2_32.a user32.a

MAIN_OBJECTS = roshttpd.o config.o error.o http.o httpd.o

COMMON_OBJECTS = common/list.o common/socket.o common/thread.o

TARGET_OBJECTS = $(MAIN_OBJECTS) $(COMMON_OBJECTS)

TARGET_CLEAN = common/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
