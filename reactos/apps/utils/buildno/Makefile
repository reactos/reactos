# $Id: Makefile,v 1.4 2000/01/22 14:25:48 ea Exp $
#

BASE_CFLAGS = -I../../include
TARGETNAME=buildno
CLEAN_FILES= $(TARGETNAME).o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).sym

all: $(TARGETNAME)$(EXE_POSTFIX)
	$(EXE_PREFIX)$(TARGETNAME)$(EXE_POSTFIX)


$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).c ../../include/reactos/version.h
	$(NATIVE_CC) -I../../include -o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).c



clean:
	$(RM) $(TARGETNAME).o
	$(RM) $(TARGETNAME).sym
	$(RM) $(TARGETNAME)$(EXE_POSTFIX)

.phony: clean 

include ../../rules.mak

# EOF
