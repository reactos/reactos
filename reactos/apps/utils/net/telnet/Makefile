# $Id: Makefile,v 1.3 2001/08/21 20:13:03 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = telnet

TARGET_SDKLIBS = crtdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = \
	ansi.o		\
	console.o	\
	helpsock.o	\
	main.o		\
	nvt.o		\
	telnet.o	\
	vm.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
