# $Id: makefile,v 1.3 2001/07/15 21:19:00 rex Exp $
#
# Local Security Authority Subsystem
#
# ReactOS Operating System
#
TARGET=lsass

BASE_CFLAGS = -I../../include

OBJECTS = $(TARGET).o init.o $(TARGET).coff

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) $(TARGET).o $(TARGET).exe $(TARGET).sym $(TARGET).coff

.phony: clean

$(TARGET).coff: $(TARGET).rc
	$(RC) $(TARGET).rc $(TARGET).coff

$(TARGET).exe: $(OBJECTS) $(LIBS)
	$(LD) \
		$(OBJECTS) \
		$(LIBS) \
		-o $(TARGET).exe \
		--subsystem native
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym

include ../../rules.mak
