# $Id: makefile,v 1.1 2000/02/20 22:52:47 ea Exp $
#
# ReactOS Operating System
#
# Generate files for a kernel module that needs to add a service table.
#
TARGETNAME = mktab

# WIN32K.SYS
SVC_DB=w32ksvc.db
SVC_MASK=0x10000000

ifeq ($(DOSCLI),yes)
SVC_SERVICE_TABLE_PATH=..\\..\\subsys\\win32k\\main\\
SVC_CLIENT_STUBS_PATH=..\\..\\lib\\gdi32\\misc\\
else
SVC_SERVICE_TABLE_PATH=../../subsys/win32k/main/
SVC_CLIENT_STUBS_PATH=../../lib/gdi32/misc/
endif

SVC_SERVICE_TABLE=$(SVC_SERVICE_TABLE_PATH)svctab.c
SVC_CLIENT_STUBS=$(SVC_CLIENT_STUBS_PATH)win32k.c

SVC_FILES = $(SVC_CLIENT_STUBS) $(SVC_SERVICE_TABLE)

BASE_CFLAGS = -I../../include

all: $(SVC_FILES)

$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).cc
	$(CC) \
		$(CFLAGS) \
		-o $(TARGETNAME) \
		-O2 \
		$(TARGETNAME).cc

$(SVC_FILES): $(SVC_DB) $(TARGETNAME)$(EXE_POSTFIX) 
	./$(TARGETNAME)$(EXE_POSTFIX) \
		$(SVC_DB) \
		$(SVC_MASK) \
		$(SVC_CLIENT_STUBS) \
		$(SVC_SERVICE_TABLE)


clean:
	- $(RM) $(TARGETNAME)$(EXE_POSTFIX)
	- $(RM) $(SVC_CLIENT_STUBS)
	- $(RM) $(SVC_SERVICE_TABLE)
	
.PHONY: all clean 

include ../../rules.mak
