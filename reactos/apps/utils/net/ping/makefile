#
# ReactOS Ping Utility
#
PATH_TO_TOP = ../../..

TARGETNAME=ping
CFLAGS = -I../../../include

OBJECTS = $(TARGETNAME).o
PROGS = $(TARGETNAME).exe
LIBS = ../../../lib/ntdll/ntdll.a \
       ../../../lib/ws2_32/ws2_32.a

CLEAN_FILES = $(TARGETNAME).o $(TARGETNAME).exe $(TARGETNAME).sym

all: $(TARGETNAME).exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(FLOPPY_DIR)\apps\$*
else
	$(CP) $* $(FLOPPY_DIR)/apps/$*
endif

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\..\$(DIST_DIR)\apps\$*
else
	$(CP) $* ../../$(DIST_DIR)/apps/$*
endif

$(TARGETNAME).exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGETNAME).exe

include ../../../rules.mak
