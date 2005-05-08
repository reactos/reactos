# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = services

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a user32.a rpcrt4.a

TARGET_OBJECTS = svcctl_s.o database.o services.o rpcserver.o

TARGET_CFLAGS = -Wall -Werror -fno-builtin \
                -D__USE_W32API \
                -D_WIN32_IE=0x0500 \
                -D_WIN32_WINNT=0x501 \
                -DWINVER=0x600 \
                -DUNICODE \
                -D_UNICODE

TARGET_CLEAN = svcctl_s.c svcctl_s.h

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

svcctl_s.c svcctl_s.h: $(PATH_TO_TOP)/include/idl/svcctl.idl
	$(WIDL) -o -h -H svcctl_s.h -s -S svcctl_s.c $(PATH_TO_TOP)/include/idl/svcctl.idl

# EOF
