# $Id: makefile,v 1.3 2000/08/12 19:33:23 dwelch Exp $
#
# Logon/login Application
#
# ReactOS Operating System
#
TARGET = winlogon

BASE_CFLAGS = -I../../include

OBJECTS = winlogon.o $(TARGET).coff

LIBS = -lkernel32 ../../lib/ntdll/ntdll.a

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

$(TARGET).exe: $(OBJECTS)
	$(CC) \
		$(OBJECTS) \
		$(LIBS) \
		-o $(TARGET).exe 
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym

include ../../rules.mak
