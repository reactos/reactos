# $Id: Makefile,v 1.1 1999/11/07 08:03:21 ea Exp $
#

BASE_CFLAGS = -I../../include
TARGETNAME=buildno
CLEAN_FILES= $(TARGETNAME).o $(TARGETNAME).exe $(TARGETNAME).sym

all: $(TARGETNAME)$(EXE_POSTFIX)
	$(TARGETNAME)$(EXE_POSTFIX) -q

$(TARGETNAME)$(EXE_POSTFIX): $(TARGETNAME).o
	$(CC) -o $(TARGETNAME)$(EXE_POSTFIX) $(TARGETNAME).o
	$(NM) --numeric-sort $(TARGETNAME)$(EXE_POSTFIX) > $(TARGETNAME).sym

$(TARGETNAME).o: $(TARGETNAME).c ../../include/reactos/version.h

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

include ../../rules.mak

# EOF
