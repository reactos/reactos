# $Id: Makefile,v 1.3 2002/08/23 12:41:55 ea Exp $
#
# PROJECT    : ReactOS Operating System
# FILE       : reactos/dk/w32/Makefile
# DESCRIPTION: Build the development kit for user mode application
# DESCRIPTION: to be run in the subsystems 6 (POSIX_GUI)
# DESCRIPTION: and 7 (POSIX_CUI).
# LICENSE    : GNU GPL V2.0
#
PATH_TO_TOP=../..

PATH_TO_PSX_TOP=$(PATH_TO_TOP)/../posix

include $(PATH_TO_TOP)/rules.mak


TARGET_FOLDER_LIB=$(PATH_TO_TOP)/dk/psx/lib
TARGET_FOLDER_INC=$(PATH_TO_TOP)/dk/psx/include

DTFLAGS = -k -l $@

MODULES_CORE=\
	$(TARGET_FOLDER_LIB)/psxdll.a\
	$(TARGET_FOLDER_LIB)/psxx.a


MODULES =\
	$(MODULES_CORE)
	
# --- Rules ---

all: $(MODULES)

# --- CORE ---

$(TARGET_FOLDER_LIB)/psxdll.a: $(PATH_TO_PSX_TOP)/lib/psxdll/psxdll.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D psxdll.dll\
		-d $<
	
$(TARGET_FOLDER_LIB)/psxx.a: $(PATH_TO_PSX_TOP)/lib/psxx/misc/psxx.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D psxx.dll\
		-d $<

# --- Service rules ---

clean:
	- $(RM) $(TARGET_FOLDER_LIB)/*.a
	- $(RM) $(TARGET_FOLDER_INC)/*.h

install:

dist:

#EOF

