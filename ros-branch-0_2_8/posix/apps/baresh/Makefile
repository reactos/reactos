# $Id: Makefile,v 1.9 2003/01/05 18:27:19 robd Exp $
#
# To run it in Win32 console mode, undefine __SUBSYSTEM_WINDOWS__
# and pass "console" in the ld's --subsystem option.
# 
#

PATH_TO_TOP = ../../../reactos

PATH_TO_PSX_TOP = ../..

TARGET_TYPE = program

#TARGET_APPTYPE = console
TARGET_APPTYPE = windows

TARGET_NAME = baresh

TARGET_CFLAGS = -D__SUBSYSTEM_WINDOWS__

TARGET_LIBS = $(PATH_TO_PSX_TOP)/lib/crt0w32.o \
	      $(PATH_TO_TOP)/dk/psx/lib/psxdll.a

TARGET_OBJECTS = sh.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
