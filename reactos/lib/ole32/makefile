# $Id: makefile,v 1.6 2001/09/02 09:10:47 chorns Exp $
#
# Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
#
# Makefile for ReactOS ole32.dll
#

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ole32

TARGET_BASE = 0x77a50000

TARGET_DEFONLY = yes

TARGET_CFLAGS = -fno-rtti -I./include

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a user32.a gdi32.a msvcrt.a

TARGET_OBJECTS = \
	DllMain.o \
	CoXxx.o \
	Moniker.o \
	Ole2.o \
	OleAuto.o \
	Misc.o \
	Storage.o \
	storage32.o \
	stg_stream.o \
	stg_bigblockfile.o \
	compobj.o \
	memlockbytes.o \
	rpcrt4_main.o \
	filemoniker.o \
	bindctx.o \
	compositemoniker.o \
	itemmoniker.o \
	antimoniker.o \
	datacache.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
