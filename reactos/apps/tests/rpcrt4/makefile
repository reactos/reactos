# $Id: makefile,v 1.1 2003/11/18 22:08:11 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API

TARGET_NAME = rpcrt4_test

TARGET_GCCLIBS = rpcrt4 msvcrt

TARGET_OBJECTS = \
rpc.o \
testlist.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
