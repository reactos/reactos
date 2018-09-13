!INCLUDE $(NTMAKEENV)\makefile.plt

STDINCLUDES=    priv.h linkload.h

TARGETSRCS0=    activeds.c

TARGETSRCS=$(TARGETSRCS0)

all: $(TARGETSRCS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

clean1: cleansrc all

cleansrc:
# Use for loop because command.com's erase doesn't support multiple
# file listings.  Separate sources into smaller lists because command.com
# faults.  Totally bogus.
    -@for %x in ($(TARGETSRCS0)) do erase %x

activeds.c : activeds.txt
    stubgen activeds.txt activeds $(STDINCLUDES) ole2.h activeds.h > $@
