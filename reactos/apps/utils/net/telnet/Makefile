# $Id: Makefile,v 1.4 2001/10/16 22:45:39 phreak Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = telnet

TARGET_SDKLIBS = kernel32.a ws2_32.a

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
