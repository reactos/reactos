#
#  ReactOS cmdutils makefile
#

TARGET=tee.exe

all: $(TARGET)

OBJECTS = tee.o

CLEAN_FILES = *.o *.exe *.sym *.coff


tee.exe: tee.o
	$(CC) tee.o -lkernel32 -lcrtdll -o tee.exe
	$(NM) --numeric-sort tee.exe > tee.sym


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


dist: $(TARGET:%=../$(DIST_DIR)/%)

$(TARGET:%=../$(DIST_DIR)/%): ../$(DIST_DIR)/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\$(DIST_DIR)\$*
else
	$(CP) $* ../$(DIST_DIR)/$*
endif

include ../rules.mak
