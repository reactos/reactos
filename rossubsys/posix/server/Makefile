# $Id: Makefile,v 1.5 2003/01/05 18:27:19 robd Exp $
#
# ReactOS POSIX+ Environment Subsystem
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_PATH = misc

TARGET_NAME = psxss

TARGET_APPTYPE = console

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_CFLAGS = -Iinclude -I../include -D__PSXSS_ON_W32__

PSXSS_MISC_OBJECTS = \
	misc/main.o \
	misc/init.o \
	misc/print.o

PSXSS_OB_OBJECTS = \
	ob/process.o \
	ob/session.o \
	ob/terminal.o

PSXSS_PORT_OBJECTS = \
	port/api.o \
	port/sbapi.o \
	port/session.o \
	port/utils.o

PSXSS_CALL_OBJECTS = \
	call/null.o \
	call/_exit.o \
	call/stubs.o \
	call/syscall.o

TARGET_OBJECTS = \
	$(PSXSS_MISC_OBJECTS) \
	$(PSXSS_OB_OBJECTS) \
	$(PSXSS_PORT_OBJECTS) \
	$(PSXSS_CALL_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
