# $Id: Makefile,v 1.3 2002/06/08 16:28:03 ea Exp $
#
# Tu run it in Win32 console mode, undefine __SUBSYSTEM_WINDOWS__
# and pass "console" in the ld's --subsystem option.
# 
#
PATH_TO_TOP=../../../reactos

PATH_TO_PSX_TOP=../..

TARGET_NAME=sh

CFLAGS=-D__SUBSYSTEM_WINDOWS__

OBJECTS=$(TARGET_NAME).o $(TARGET_NAME).coff

LIBRARIES=\
	$(PATH_TO_PSX_TOP)/lib/crt0w32.o \
	$(PATH_TO_TOP)/dk/psx/lib/psxdll.a

$(TARGET_NAME): $(OBJECTS) $(LIBRARIES)
	$(CC) \
		$(CFLAGS) \
		$(OBJECTS) \
		$(LIBRARIES)\
		-o $@ \
		-Wl,--subsystem,windows\
		-nostartfiles \
		-nostdlib

$(TARGET_NAME).coff: $(TARGET_NAME).rc
	$(RC) \
		--include-dir $(PATH_TO_TOP)/include \
		--output-format coff \
		$< $@ 
		

include $(PATH_TO_TOP)/rules.mak

# EOF
