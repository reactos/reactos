# $Id: makefile,v 1.9 2002/09/07 15:12:45 chorns Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = version

TARGET_BASE = 0x77000000

TARGET_SDKLIBS = kernel32.a

TARGET_OBJECTS =  misc/libmain.o misc/stubs.o

TARGET_CLEAN = misc/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
