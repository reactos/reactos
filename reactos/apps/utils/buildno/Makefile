# $Id: Makefile,v 1.9 2001/04/11 22:13:21 dwelch Exp $
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
	- $(RM) $(TARGETNAME).o
	- $(RM) $(TARGETNAME).sym
	- $(RM) $(TARGETNAME)$(EXE_POSTFIX)

.phony: clean 

include ../../rules.mak

# EOF
