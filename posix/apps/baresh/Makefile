# $Id: Makefile,v 1.1 2002/01/20 21:22:29 ea Exp $
#
# Tu run it in Win32 console mode, undefine __SUBSYSTEM_WINDOWS__
# and pass "console" in the ld's --subsystem option.
# 
#
PATH_TO_TOP=../../../..

PATH_TO_PSX_TOP=../..

TARGET_NAME=sh

CFLAGS=-D__SUBSYSTEM_WINDOWS__

OBJECTS=$(TARGET_NAME).o

LIBRARIES=\
	$(PATH_TO_PSX_TOP)/lib/crt0w32.o \
	$(PATH_TO_PSX_TOP)/lib/psxdll/psxdll.a

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
