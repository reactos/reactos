#
#
#
PATH_TO_TOP = ../..

SRV_OBJECTS = ../common/crt0.o msserver.o
CLT_OBJECTS = ../common/crt0.o msclient.o

PROGS = msserver.exe msclient.exe

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

msserver.exe: $(SRV_OBJECTS) $(LIBS)
	$(LD) $(SRV_OBJECTS) $(LIBS) -o msserver.exe
	$(NM) --numeric-sort msserver.exe > msserver.sym

msclient.exe: $(CLT_OBJECTS) $(LIBS)
	$(LD) $(CLT_OBJECTS) $(LIBS) -o msclient.exe
	$(NM) --numeric-sort msclient.exe > msclient.sym


include ../../rules.mak
