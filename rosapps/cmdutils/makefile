#
#  ReactOS cmdutils makefile
#

PATH_TO_TOP=..

TARGET=find.exe more.exe sort.exe tee.exe y.exe

all: $(TARGET)


CLEAN_FILES = *.o *.exe *.sym *.coff

find.exe: find.o find.coff
	$(CC) find.o -lkernel32 -lmsvcrt -o find.exe find.coff
	$(NM) --numeric-sort find.exe > find.sym

more.exe: more.o more.coff
	$(CC) more.o -lkernel32 -lmsvcrt -o more.exe more.coff
	$(NM) --numeric-sort more.exe > more.sym

sort.exe: sort.o sort.coff
	$(CC) sort.o -lkernel32 -lmsvcrt -o sort.exe sort.coff
	$(NM) --numeric-sort sort.exe > sort.sym

tee.exe: tee.o tee.coff
	$(CC) tee.o -lkernel32 -lmsvcrt -o tee.exe tee.coff
	$(NM) --numeric-sort tee.exe > tee.sym

y.exe: y.o y.coff
	$(CC) y.o -lkernel32 -lmsvcrt -o y.exe y.coff
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


dist: $(TARGET:%=$(PATH_TO_TOP)/$(DIST_DIR)/apps/%)

$(TARGET:%=$(PATH_TO_TOP)/$(DIST_DIR)/apps/%): $(PATH_TO_TOP)/$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(PATH_TO_TOP)\$(DIST_DIR)\apps\$*
else
	$(CP) $* $(PATH_TO_TOP)/$(DIST_DIR)/apps/$*
endif

include $(PATH_TO_TOP)/rules.mak

