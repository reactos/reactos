# $Id: makefile 12852 2005-01-06 13:58:04Z mf $

PATH_TO_TOP = ../..

TARGET_TYPE = winedll

TARGET_CLEAN = pnp_c.c pnp.h

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

WIDL_FLAGS = -o \
             -D _X86_ -D MIDL_PASS \
             -I $(PATH_TO_TOP)/w32api/include

pnp_c.c pnp.h: $(PATH_TO_TOP)/include/idl/pnp.idl
	$(WIDL) $(WIDL_FLAGS) -h -H pnp.h -c -C pnp_c.c $(PATH_TO_TOP)/include/idl/pnp.idl
