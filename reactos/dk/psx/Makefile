# $Id: Makefile,v 1.1 2001/12/15 11:04:26 ea Exp $
#
# PROJECT    : ReactOS Operating System
# FILE       : reactos/dk/w32/Makefile
# DESCRIPTION: Build the development kit for user mode application
# DESCRIPTION: to be run in the subsystems 6 (POSIX_GUI)
# DESCRIPTION: and 7 (POSIX_CUI).
# LICENSE    : GNU GPL V2.0
#
PATH_TO_TOP=../..

include $(PATH_TO_TOP)/rules.mak


TARGET_FOLDER_LIB=lib
TARGET_FOLDER_INC=include

DTFLAGS = -k -l $@

MODULES_CORE=\
	$(TARGET_FOLDER_LIB)/psxdll.a\
	$(TARGET_FOLDER_LIB)/psxx.a


MODULES =\
	$(MODULES_CORE)
	
# --- Rules ---

all: $(MODULES)

# --- CORE ---

$(TARGET_FOLDER_LIB)/psxdll.a: $(PATH_TO_TOP)/subsys/psx/lib/psxdll/misc/psxdll.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D psxdll.dll\
		-d $(PATH_TO_TOP)/subsys/psx/lib/psxdll/misc/psxdll.def
	
$(TARGET_FOLDER_LIB)/psxx.a: $(PATH_TO_TOP)/subsys/psx/lib/psxx/misc/psxx.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D psxx.dll\
		-d $(PATH_TO_TOP)/subsys/psx/lib/psxx/misc/psxx.def

# --- Service rules ---

clean:
	- $(RM) $(TARGET_FOLDER_LIB)/*.a
	- $(RM) $(TARGET_FOLDER_INC)/*.h

install:

dist:

#EOF

