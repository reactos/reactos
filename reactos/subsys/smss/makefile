# $Id: makefile,v 1.4 2000/08/11 12:46:37 ekohl Exp $
#
# Session Manager
#
# ReactOS Operating System
#
TARGET=smss

BASE_CFLAGS = -I../../include

OBJECTS = $(TARGET).o init.o smapi.o $(TARGET).coff

LIBS = ../../lib/ntdll/ntdll.a

CLEAN_FILES = *.o $(TARGET).exe $(TARGET).sym $(TARGET).coff

all: $(TARGET).exe

.phony: all


$(TARGET).coff: $(TARGET).rc

$(TARGET).exe: $(OBJECTS) $(LIBS)
	$(LD) \
		$(OBJECTS) \
		$(LIBS) \
		-o $(TARGET).exe \
		--subsystem native
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)


install: $(FLOPPY_DIR)/subsys/$(TARGET).exe

$(FLOPPY_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).exe $(FLOPPY_DIR)\subsys\$(TARGET).exe
else
	$(CP) $(TARGET).exe $(FLOPPY_DIR)/subsys/$(TARGET).exe
endif


dist: $(DIST_DIR)/subsys/$(TARGET).exe

$(DIST_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).exe ..\..\$(DIST_DIR)\subsys\$(TARGET).exe
else
	$(CP) $(TARGET).exe ../../$(DIST_DIR)/subsys/$(TARGET).exe
endif

include ../../rules.mak
