# $Id: makefile,v 1.3 2004/06/01 01:13:29 sedwards Exp $
#
# Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
#
# Makefile for ReactOS regsvr32.dll
#

PATH_TO_TOP=../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = regsvr32

TARGET_SDKLIBS = kernel32.a

TARGET_CFLAGS = -D__USE_W32API

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
