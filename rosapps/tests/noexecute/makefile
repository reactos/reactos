#
# $Id: makefile,v 1.1 2004/10/21 04:59:00 sedwards Exp $
#

PATH_TO_TOP = ../../../reactos

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = noexecute

TARGET_SDKLIBS = pseh.a

TARGET_PCH = 

TARGET_OBJECTS = \
	noexecute.o 

TARGET_CFLAGS =   -Wall -Werror -D__USE_W32API

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
