# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = umpnpmgr

TARGET_INSTALLDIR = system32

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a rpcrt4.a

TARGET_OBJECTS = umpnpmgr.o pnp_s.o

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DUNICODE -D_UNICODE

TARGET_CLEAN = pnp_s.c pnp.h

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

WIDL_FLAGS = \
             -D _X86_ -D MIDL_PASS \
             -I $(PATH_TO_TOP)/w32api/include

pnp_s.c pnp.h: $(PATH_TO_TOP)/include/idl/pnp.idl
	$(WIDL) $(WIDL_FLAGS) -h -H pnp.h -s -S pnp_s.c $(PATH_TO_TOP)/include/idl/pnp.idl

# EOF
