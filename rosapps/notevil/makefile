# $Id: makefile,v 1.1 1999/05/15 07:23:34 ea Exp $
#
# ReactOS makefile for notevil
# Compiler: egcs 1.1.2
#
TARGET=notevil

all: $(TARGET).exe

OBJECTS = $(TARGET).o $(TARGET).coff

CLEAN_FILES = *.o $(TARGET).exe $(TARGET).sym $(TARGET).coff

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

$(TARGET).exe: $(OBJECTS)
	$(CC) $(OBJECTS) \
		-o $(TARGET).exe \
		-lkernel32 \
		-luser32 \
		-lcrtdll 
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym
	
$(TARGET).coff: $(TARGET).rc
	$(RC) $(RFLAGS) $(TARGET).rc $(TARGET).coff
	
include ../rules.mak

# EOF
