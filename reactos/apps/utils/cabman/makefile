#
# Makefile for ReactOS Cabinet Manager
#
PATH_TO_TOP = ../..

ZLIB_OBJECTS    = zlib/adler32.o zlib/deflate.o zlib/infblock.o zlib/infcodes.o \
                  zlib/inflate.o zlib/inftrees.o zlib/infutil.o zlib/inffast.o \
                  zlib/trees.o zlib/zutil.o
ENGINE_OBJECTS  = $(ZLIB_OBJECTS) cabinet.o mszip.o raw.o
TEST_OBJECTS    = $(ENGINE_OBJECTS) test.o
OBJECTS         = $(ENGINE_OBJECTS) main.o dfp.o
TARGET          = cabman
PROGS           = $(TARGET).exe test.exe

CFLAGS 		+= -O3

CLEAN_FILES = *.o zlib/*.o $(TARGET).exe $(TARGET).sym test.exe test.sym


all: $(PROGS)

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

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

$(TARGET).exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET).exe
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym
	
test.exe: $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o test.exe
	$(NM) --numeric-sort test.exe > test.sym

include ../../rules.mak

