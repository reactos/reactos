# $Id: Makefile,v 1.4 2004/01/12 22:45:53 sedwards Exp $

PATH_TO_TOP=../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = telnet

TARGET_SDKLIBS = ws2_32.a

TARGET_CPPFLAGS = -D__REACTOS__ -D__USE_W32API

TARGET_GCCLIBS = stdc++

TARGET_OBJECTS = \
	src/ansiprsr.o \
	src/keytrans.o \
	src/tcharmap.o \
	src/tconsole.o \
	src/tkeydef.o \
	src/tkeymap.o \
	src/tmapldr.o \
	src/tmouse.o \
	src/tnclass.o \
	src/tnclip.o \
	src/tncon.o \
	src/tnconfig.o \
	src/tnerror.o \
	src/tnetwork.o \
	src/tnmain.o \
	src/tnmisc.o \
	src/tscript.o \
	src/tscroll.o \
	src/ttelhndl.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
