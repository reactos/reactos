#
#
#
PROGS = gditest

OBJECTS= ../common/crt0.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/gdi32/gdi32.a
#LIBS= ../../lib/kernel32/kernel32.a ../../lib/crtdll/crtdll.a
BASE_CFLAGS = -I../../include

all: $(PROGS:%=%.exe)

.phony: all

$(PROGS:%=%_clean): %_clean:
	- $(RM) $*.o
	- $(RM) $*.exe
	- $(RM) $*.sym

clean: $(PROGS:%=%_clean)

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
ifeq ($(DOSCLI),yes)
	$(CP) $*.exe $(FLOPPY_DIR)\apps\$*.exe
else
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe
endif

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
ifeq ($(DOSCLI),yes)
	$(CP) $*.exe ..\..\$(DIST_DIR)\apps\$*.exe
else
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe
endif

gditest.exe: gditest.c
	$(CC) -specs=../../specs gditest.c $(BASE_CFLAGS) $(OBJECTS) $(LIBS) -lgcc -o gditest.exe
	$(NM) --numeric-sort gditest.exe > gditest.sym

include ../../rules.mak
