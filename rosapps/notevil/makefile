# $Id: makefile,v 1.3 2000/09/12 23:21:20 ea Exp $
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

$(TARGET).o: $(TARGET).c resource.h

floppy: $(TARGET:%=$(FLOPPY_DIR)/apps/%)

$(TARGET:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(FLOPPY_DIR)\apps\$*
else
	$(CP) $* $(FLOPPY_DIR)/apps/$*
endif


dist: $(TARGET:%=../$(DIST_DIR)/apps/%)

$(TARGET:%=../$(DIST_DIR)/apps/%): ../$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\$(DIST_DIR)\apps\$*
else
	$(CP) $* ../$(DIST_DIR)/apps/$*
endif

include ../rules.mak

# EOF
