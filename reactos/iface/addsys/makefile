# $Id: makefile,v 1.4 2000/06/04 17:14:25 ekohl Exp $
#
# ReactOS Operating System
#
# Generate files for a kernel module that needs to add a service table.
#
#TARGETNAME = mktab
TARGETNAME = genw32k

# WIN32K.SYS
SVC_DB=w32ksvc.db
SVC_MASK=0x1000

ifeq ($(DOSCLI),yes)
SVC_SERVICE_TABLE_PATH=..\..\subsys\win32k\main\
SVC_GDI_STUBS_PATH=..\..\lib\gdi32\misc\
SVC_USER_STUBS_PATH=..\..\lib\user32\misc\
else
SVC_SERVICE_TABLE_PATH=../../subsys/win32k/main/
SVC_GDI_STUBS_PATH=../../lib/gdi32/misc/
SVC_USER_STUBS_PATH=../../lib/user32/misc/
endif

SVC_SERVICE_TABLE=$(SVC_SERVICE_TABLE_PATH)svctab.c
SVC_GDI_STUBS=$(SVC_GDI_STUBS_PATH)win32k.c
SVC_USER_STUBS=$(SVC_USER_STUBS_PATH)win32k.c

SVC_FILES = $(SVC_GDI_STUBS) $(SVC_USER_STUBS) $(SVC_SERVICE_TABLE)

BASE_CFLAGS = -I../../include

all: $(SVC_FILES)

$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c
	$(NATIVE_CC) \
		$(CFLAGS) \
		-o $(TARGETNAME) \
		-O2 \
		$(TARGETNAME).c

#$(SVC_FILES): $(SVC_DB) $(TARGETNAME)$(EXE_POSTFIX)
#	./$(TARGETNAME)$(EXE_POSTFIX) \
#		$(SVC_DB) \
#		$(SVC_MASK) \
#		$(SVC_CLIENT_STUBS) \
#		$(SVC_SERVICE_TABLE)

$(SVC_FILES): $(SVC_DB) $(TARGETNAME)$(EXE_POSTFIX) 
	./$(TARGETNAME)$(EXE_POSTFIX) \
		$(SVC_DB) \
		$(SVC_SERVICE_TABLE) \
		$(SVC_GDI_STUBS) \
		$(SVC_USER_STUBS)


clean:
	- $(RM) $(TARGETNAME)$(EXE_POSTFIX)
	- $(RM) $(SVC_GDI_STUBS)
	- $(RM) $(SVC_USER_STUBS)
	- $(RM) $(SVC_SERVICE_TABLE)
	
.PHONY: all clean 

include ../../rules.mak
