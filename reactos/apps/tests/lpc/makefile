#
#
#
PATH_TO_TOP = ../..

SRV_OBJECTS= ../common/crt0.o lpcsrv.o
CLT_OBJECTS= ../common/crt0.o lpcclt.o

PROGS= lpcsrv.exe lpcclt.exe

BASE_CFLAGS = -I../../include
LIBS = ../../lib/crtdll/crtdll.a ../../lib/kernel32/kernel32.a \
       ../../lib/ntdll/ntdll.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) lpcsrv.o
	- $(RM) lpcsrv.exe
	- $(RM) lpcsrv.sym
	- $(RM) lpcclt.o
	- $(RM) lpcclt.exe
	- $(RM) lpcsrv.sym
	
.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

lpcsrv.exe: $(SRV_OBJECTS) $(LIBS)
	$(LD) $(SRV_OBJECTS) $(LIBS) -o lpcsrv.exe
	$(NM) --numeric-sort lpcsrv.exe > lpcsrv.sym

lpcclt.exe: $(CLT_OBJECTS) $(LIBS)
	$(LD) $(CLT_OBJECTS) $(LIBS) -o lpcclt.exe
	$(NM) --numeric-sort lpcclt.exe > lpcclt.sym


include ../../rules.mak
