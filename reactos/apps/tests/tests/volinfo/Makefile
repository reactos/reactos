#
#
#
PATH_TO_TOP = ../../../..

PROGS = volinfo

OBJECTS = volinfo.o

LIBS =
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o  *.exe *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

volinfo.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o volinfo.exe
	$(NM) --numeric-sort volinfo.exe > volinfo.sym

include $(PATH_TO_TOP)/rules.mak
