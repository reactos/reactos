# $Id: makefile 12852 2005-01-06 13:58:04Z mf $
#
# ReactOS Operating System
#
# Generate files for a kernel module that needs to add a service table.
#
PATH_TO_TOP = ../..

TARGETNAME = ncitool

# WIN32K.SYS
WIN32K_SVC_DB = w32ksvc.db
WIN32K_SERVICE_TABLE=../../subsys/win32k/main/svctab.c
WIN32K_GDI_STUBS=../../lib/gdi32/misc/win32k.S
WIN32K_USER_STUBS=../../lib/user32/misc/win32k.S

# NTOSKRNL.EXE
KERNEL_SVC_DB = sysfuncs.lst
KERNEL_SERVICE_TABLE = ../../include/ntdll/napi.h
NTDLL_STUBS = ../../lib/ntdll/napi.S
KERNEL_STUBS = ../../ntoskrnl/ex/zw.S

SVC_FILES =  $(KERNEL_SERVICE_TABLE) $(WIN32K_SERVICE_TABLE) $(NTDLL_STUBS) $(KERNEL_STUBS) $(WIN32K_GDI_STUBS) $(WIN32K_USER_STUBS)

CLEAN_FILES = $(TARGETNAME)$(EXE_POSTFIX) $(SVC_FILES)

BASE_CFLAGS = -I../../include

CFLAGS += -Wall -Werror

all: $(SVC_FILES)

$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c
	$(HOST_CC) \
		$(CFLAGS) \
		-o $(TARGETNAME) \
		-O2 \
		$(TARGETNAME).c

$(SVC_FILES): $(TARGETNAME)$(EXE_POSTFIX)
	@./$(TARGETNAME)$(EXE_POSTFIX) \
		$(KERNEL_SVC_DB) \
		$(WIN32K_SVC_DB) \
		$(KERNEL_SERVICE_TABLE) \
		$(WIN32K_SERVICE_TABLE) \
		$(NTDLL_STUBS) \
		$(KERNEL_STUBS) \
		$(WIN32K_GDI_STUBS) \
		$(WIN32K_USER_STUBS)

clean:
	- $(RM) $(CLEAN_FILES)
	
.PHONY: all clean 

include ../../rules.mak
