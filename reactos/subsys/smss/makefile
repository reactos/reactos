# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_SDKLIBS = smlib.a ntdll.a

TARGET_NAME = smss

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__NTAPP__

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_OBJECTS = $(TARGET_NAME).o \
		 init.o initheap.o initenv.o initobdir.o initdosdev.o \
		 initrun.o initmv.o initwkdll.o initpage.o initss.o \
		 initreg.o \
		 smapi.o smapicomp.o smapiexec.o smapiquery.o \
		 client.o debug.o print.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
