# $Id: Makefile,v 1.7 2001/08/21 20:13:07 chorns Exp $


PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = secur32

TARGET_BASE = 0x10000000

TARGET_SDKLIBS = ntdll.a

TARGET_CFLAGS = -I./include -D__SECUR32__

TARGET_OBJECTS = lsa.o dllmain.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
