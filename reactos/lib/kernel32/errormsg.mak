# $Id: errormsg.mak,v 1.1 2002/12/15 06:34:42 robd Exp $

PATH_TO_TOP = ../..

#TARGET_TYPE = dynlink

TARGETNAME = kernel32

errcodes.rc: $(TARGETNAME).mc
	$(MC) \
		-H $(PATH_TO_TOP)/include/reactos/errcodes.h \
		-o errcodes.rc \
		$(TARGETNAME).mc

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

