#
#
#
OBJECTS= ../common/crt0.o objdir.o


PROGS= objdir.exe 

BASE_CFLAGS = -I../../include
LIB_BASE=../../lib
LIBS = \
	$(LIB_BASE)/ntdll/ntdll.a \
	$(LIB_BASE)/kernel32/kernel32.a \
	$(LIB_BASE)/crtdll/crtdll.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) objdir.o
	- $(RM) objdir.exe
	- $(RM) objdir.sym

.phony: clean

floppy: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

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

objdir.exe: $(OBJECTS) $(LIBS)
	$(LD) $(OBJECTS) $(LIBS) -o objdir.exe
	$(NM) --numeric-sort objdir.exe > objdir.sym


include ../../rules.mak
