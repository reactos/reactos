# $Id: Makefile,v 1.1 2001/01/27 22:38:43 ea Exp $
#
# ReactOS Network Virtual Terminal (telnet) console client
#
PATH_TO_TOP=../../..
PATH_TO_LIB=$(PATH_TO_TOP)/lib

TARGET_NAME=telnet

OBJECTS=\
	ansi.o		\
	console.o	\
	helpsock.o	\
	main.o		\
	nvt.o		\
	telnet.o	\
	vm.o		\
	$(TARGET_NAME).coff

LIBRARIES=\
	$(PATH_TO_LIB)/crtdll/crtdll.a		\
	$(PATH_TO_LIB)/kernel32/kernel32.a	\
	$(PATH_TO_LIB)/ws2_32/ws2_32.a

PROGS=$(TARGET_NAME).exe
	
ifeq ($(DOSCLI), yes)
CLEAN_FILES = *.o $(TARGET_NAME).exe $(TARGET_NAME).sym
else
CLEAN_FILES = *.o $(TARGET_NAME).exe $(TARGET_NAME).sym
endif

all: $(TARGET_NAME).exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

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

ansi.o: telnet.h

helpsock.o: telnet.h

main.o: telnet.h

nvt.o: telnet.h

telnet.o: telnet.h

telnet.coff: $(PATH_TO_TOP)/include/reactos/resource.h

vm.o: telnet.h

$(TARGET_NAME).exe: $(OBJECTS) $(LIBRARIES)
	$(CC) $(OBJECTS) $(LIBRARIES) -o $(TARGET_NAME).exe

include $(PATH_TO_TOP)/rules.mak

# EOF
