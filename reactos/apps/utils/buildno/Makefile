# $Id: Makefile,v 1.6 2000/06/29 23:35:09 dwelch Exp $
#

BASE_CFLAGS = 
TARGETNAME=buildno
CLEAN_FILES= $(TARGETNAME).o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).sym

all: $(TARGETNAME)$(EXE_POSTFIX)
	$(EXE_PREFIX)$(TARGETNAME)$(EXE_POSTFIX)


$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c ../../include/reactos/version.h
	$(NATIVE_CC) -o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).c



clean:
	- $(RM) $(TARGETNAME).o
	- $(RM) $(TARGETNAME).sym
	- $(RM) $(TARGETNAME)$(EXE_POSTFIX)

.phony: clean 

include ../../rules.mak

# EOF
