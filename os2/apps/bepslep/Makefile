# $Id: Makefile,v 1.1 2002/03/23 19:23:27 robertk Exp $
#
# To run it in Win32 console mode, undefine __SUBSYSTEM_WINDOWS__
# and pass "console" in the ld's --subsystem option.
# 
#
PATH_TO_TOP=../../../..

PATH_TO_OS2_TOP=../..

TARGET_NAME=bepslep

CFLAGS=-D__SUBSYSTEM_WINDOWS__

OBJECTS=$(TARGET_NAME).o

LIBRARIES=\
	$(PATH_TO_OS2_TOP)/dll/doscalls.dll 

$(TARGET_NAME): $(OBJECTS) $(LIBRARIES)
	$(CC) \
		$(CFLAGS) \
		$(OBJECTS) \
		$(LIBRARIES)\
		-o $@ \
		-Wl,--subsystem,windows\
		-nostartfiles \
		-nostdlib

include $(PATH_TO_TOP)/rules.mak

# EOF
