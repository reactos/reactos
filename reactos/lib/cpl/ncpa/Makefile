# $Id: Makefile,v 1.3 2004/10/31 11:54:58 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = dynlink

TARGET_EXTENSION = .cpl

TARGET_NAME = ncpa

TARGET_INSTALLDIR = system32

TARGET_BASE = $(TARGET_BASE_LIB_CPL_NCPA)

TARGET_CFLAGS = \
 -I./include \
 -D_WIN32_IE=0x0600 \
 -D_WIN32_WINNT=0x0501 \
 -D__USE_W32API \
 -DUNICODE \
 -D_UNICODE \
 -D__REACTOS__ \
 -Wall \
 -Werror \
 -fno-builtin

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = kernel32.a user32.a comctl32.a iphlpapi.a

TARGET_GCCLIBS = gcc

TARGET_PCH = 

TARGET_CLEAN = 

TARGET_OBJECTS = ncpa.o tcpip_properties.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: ./TAGS
