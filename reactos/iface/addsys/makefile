# $Id: makefile,v 1.11 2002/08/26 13:06:02 dwelch Exp $
#
# ReactOS Operating System
#
# Generate files for a kernel module that needs to add a service table.
#
PATH_TO_TOP = ../..

#TARGETNAME = mktab
TARGETNAME = genw32k

# WIN32K.SYS
SVC_DB=w32ksvc.db
SVC_MASK=0x1000

# DOS and Win32 kernels handle Unix paths too.
SVC_SERVICE_TABLE=../../subsys/win32k/main/svctab.c
SVC_GDI_STUBS=../../lib/gdi32/misc/win32k.c
SVC_USER_STUBS=../../lib/user32/misc/win32k.c
SVC_CSRSS_STUBS=../../subsys/csrss/win32k.c

SVC_FILES = $(SVC_GDI_STUBS) $(SVC_USER_STUBS) $(SVC_SERVICE_TABLE)

CLEAN_FILES = $(TARGETNAME)$(EXE_POSTFIX) $(SVC_FILES)

BASE_CFLAGS = -I../../include

all: $(SVC_FILES)

$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c
	$(HOST_CC) \
		$(CFLAGS) \
		-o $(TARGETNAME) \
		-O2 \
		$(TARGETNAME).c

$(SVC_FILES): $(SVC_DB) $(TARGETNAME)$(EXE_POSTFIX)
	./$(TARGETNAME)$(EXE_POSTFIX) \
		$(SVC_DB) \
		$(SVC_SERVICE_TABLE) \
		$(SVC_GDI_STUBS) \
		$(SVC_USER_STUBS) \
		$(SVC_CSRSS_STUBS)


clean:
	- $(RM) $(CLEAN_FILES)
	
.PHONY: all clean 

include ../../rules.mak
