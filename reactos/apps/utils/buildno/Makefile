# $Id: Makefile,v 1.8 2000/10/07 13:41:44 dwelch Exp $
#

PATH_TO_TOP = ../..

BASE_CFLAGS = 
TARGETNAME=buildno
CLEAN_FILES= $(TARGETNAME).o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).sym

all: $(TARGETNAME)$(EXE_POSTFIX)
	$(EXE_PREFIX)$(TARGETNAME)$(EXE_POSTFIX)


$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c ../../include/reactos/version.h
	$(HOST_CC) -o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).c



clean:
	- $(RM) $(TARGETNAME).o
	- $(RM) $(TARGETNAME).sym
	- $(RM) $(TARGETNAME)$(EXE_POSTFIX)

.phony: clean 

include ../../rules.mak

# EOF
