#
#
#
PATH_TO_TOP = ../..

PROGS = notepad

OBJECTS = dialog.o language.o license.o License_En.o main.o search.o rsrc.coff

LIBS = ../../dk/w32/lib/kernel32.a ../../dk/w32/lib/gdi32.a \
../../dk/w32/lib/libcomdlg32.a
       
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

notepad.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o notepad.exe
	$(NM) --numeric-sort notepad.exe > notepad.sym

%.coff: %.rc
	$(RC) $(RCINC) $< -o $@

include ../../rules.mak
