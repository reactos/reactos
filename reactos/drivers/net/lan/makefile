# $Id$

PATH_TO_TOP = ../../..

# TARGET_REGTESTS = yes

TARGET_TYPE = driver

TARGET_NAME = lan

TARGET_PCH = include/precomp.h

# -DMEMTRACK
TARGET_CFLAGS = \
	-D__USE_W32API \
	-D_SEH_NO_NATIVE_NLG \
	-DNDIS40 \
	-DMEMTRACK \
	-Wall -Werror \
	-I./include \
	-I$(PATH_TO_TOP)/w32api/include \
	-I$(PATH_TO_TOP)/include

TARGET_DDKLIBS = ndis.a

TARGET_LIBS = \
	$(PATH_TO_TOP)/dk/w32/lib/libpseh.a

TARGET_CLEAN = \
	lan/*.o

TARGET_OBJECTS = lan/lan.o lan/routines.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
