# $Id: Makefile,v 1.1 2004/09/09 04:18:30 sedwards Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = user32_test

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a

TARGET_CFLAGS = -D__USE_W32API -I../shared -Wall

TARGET_OBJECTS = \
  dde.o \
  testlist.o \
  wsprintf.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
