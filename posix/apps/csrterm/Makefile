# $Id: Makefile,v 1.1 2002/03/17 22:15:39 ea Exp $
#
# POSIX+ Terminal Emulator that runs in the W32 subsystem.
#
PATH_TO_TOP=../../../..
PATH_TO_TOP_PSX=../..

TARGET_NAME=csrterm
TARGET_TYPE=program
TARGET_APPTYPE=console
TARGET_CFLAGS =-I$(PATH_TO_TOP_PSX)/include
TARGET_SDKLIBS=ntdll.a kernel32.a
TARGET_OBJECTS=\
	$(TARGET_NAME).o \
	console.o \
	vt100.o \
	$(TARGET_NAME).coff

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
