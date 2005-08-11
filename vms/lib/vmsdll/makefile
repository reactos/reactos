# $Id$
# 
PATH_TO_TOP = ../../../reactos

TARGET_TYPE = dynlink

TARGET_NAME = vmsdll

TARGET_CFLAGS = -Wall -Werror -fno-builtin \
                -D__USE_W32API \
                -D_WIN32_IE=0x0500 \
                -D_WIN32_WINNT=0x501 \
                -DWINVER=0x600 \

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a

LIB_OBJECTS = \
	lib\stubs.o

RMS_OBJECTS = \
	rms\stubs.o

MISC_OBJECTS = \
	dllmain.o

SYS_OBJECTS = \
	sys\stubs.o

TARGET_OBJECTS = \
	$(LIB_OBJECTS) \
	$(RMS_OBJECTS) \
	$(SYS_OBJECTS) \
	$(MISC_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

