#
#  ReactOS cmdutils makefile
#

TARGET=more.exe tee.exe y.exe

all: $(TARGET)


CLEAN_FILES = *.o *.exe *.sym *.coff

more.exe: more.o
	$(CC) more.o -lkernel32 -lcrtdll -o more.exe
	$(NM) --numeric-sort more.exe > more.sym

tee.exe: tee.o
	$(CC) tee.o -lkernel32 -lcrtdll -o tee.exe
	$(NM) --numeric-sort tee.exe > tee.sym

y.exe: y.o
	$(CC) y.o -lkernel32 -lcrtdll -o y.exe
	$(NM) --numeric-sort y.exe > y.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)


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
