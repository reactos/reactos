# $Id: makefile,v 1.2 1999/08/29 13:45:07 dwelch Exp $
#
# CSRSS: Client/server runtime subsystem
#
# ReactOS Operating System
#
TARGET=csrss

BASE_CFLAGS = -I../../include

OBJECTS_API = api/process.o

OBJECTS_SBAPI =

OBJECTS_MISC = $(TARGET).o init.o lpcstub.o $(TARGET).coff 

OBJECTS = $(OBJECTS_API) $(OBJECTS_SBAPI) $(OBJECTS_MISC)

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) api/*.o
	- $(RM) sbapi/*.o
	- $(RM) *.o
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

# EOF
