#
#
#
PATH_TO_TOP = ../..

SRV_OBJECTS = ../common/crt0.o npserver.o
CLT_OBJECTS = ../common/crt0.o npclient.o

PROGS = npserver.exe npclient.exe

BASE_CFLAGS = -I../../include
LIBS = ../../lib/crtdll/crtdll.a ../../lib/kernel32/kernel32.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) *.o
	- $(RM) *.exe
	- $(RM) *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

npserver.exe: $(SRV_OBJECTS) $(LIBS)
	$(LD) $(SRV_OBJECTS) $(LIBS) -o npserver.exe
	$(NM) --numeric-sort npserver.exe > npserver.sym

npclient.exe: $(CLT_OBJECTS) $(LIBS)
	$(LD) $(CLT_OBJECTS) $(LIBS) -o npclient.exe
	$(NM) --numeric-sort npclient.exe > npclient.sym


include ../../rules.mak
