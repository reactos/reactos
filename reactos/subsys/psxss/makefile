# $Id: makefile,v 1.3 1999/09/07 17:12:39 ea Exp $
#
# PSXSS: POSIX+ Subsystem server process
#
# ReactOS Operating System
#
TARGET=psxss

OBJECTS_API = api/process.o api/thread.o api/subsys.o

OBJECTS_MISC = $(TARGET).o init.o $(TARGET).coff

OBJECTS =  $(OBJECTS_MISC) $(OBJECTS_API)

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) *.o
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff
	- $(RM) api/*.o

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
