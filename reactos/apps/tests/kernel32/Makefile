# $Id: Makefile,v 1.2 2004/05/04 17:25:18 navaraf Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API -DWINVER=0x0500

TARGET_NAME = kernel32_test

TARGET_SDKLIBS = kernel32.a

TARGET_OBJECTS = \
	testlist.o \
	atom.o \
	change.o \
	codepage.o \
	comm.o \
	console.o \
	directory.o \
	drive.o \
	environ.o \
	file.o \
	format_msg.o \
	heap.o \
	locale.o \
	mailslot.o \
	path.o \
	pipe.o \
	process.o \
	profile.o \
	thread.o \
	virtual.o

#FIXME	alloc.o \

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
