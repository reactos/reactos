# $Id: Makefile,v 1.10 2001/07/15 21:18:49 rex Exp $
#

PATH_TO_TOP = ../..

BASE_CFLAGS = 
TARGETNAME=buildno
CLEAN_FILES= $(TARGETNAME).o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).sym

all: $(TARGETNAME)$(EXE_POSTFIX)
	$(EXE_PREFIX)$(TARGETNAME)$(EXE_POSTFIX)


$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c ../../include/reactos/version.h
	$(HOST_CC) -g -o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).c



clean:
	- $(RM) $(CLEAN_FILES)

.phony: clean 

include ../../rules.mak

# EOF
