# $Id: makefile,v 1.2 1999/08/29 13:45:06 dwelch Exp $
#
# Session Manager
#
# ReactOS Operating System
#
TARGET=smss

BASE_CFLAGS = -I../../include

OBJECTS = $(TARGET).o init.o $(TARGET).coff

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) $(TARGET).o
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

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
