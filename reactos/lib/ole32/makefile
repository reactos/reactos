# $Id: makefile,v 1.5 2001/08/21 20:13:07 chorns Exp $
#
# Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
#
# Makefile for ReactOS ole32.dll
#

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ole32

TARGET_BASE = 0x77a50000

TARGET_CFLAGS = -fno-rtti

TARGET_SDKLIBS = kernel32.a

TARGET_OBJECTS = \
	DllMain.o \
	CoXxx.o \
	Moniker.o \
	Ole2.o \
	OleAuto.o \
	Misc.o \
	Storage.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
