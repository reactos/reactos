#
# Makefile for ReactOS Cabinet Manager
#
PATH_TO_TOP = ../../..

#FIXME: why doesn't this work?
#ZLIB_OBJECTS    = $(PATH_TO_TOP)/drivers/lib/zlib/zlib.a
ZLIB_PATH = $(PATH_TO_TOP)/drivers/lib/zlib

ZLIB_OBJECTS    = $(ZLIB_PATH)/adler32.o $(ZLIB_PATH)/deflate.o \
                  $(ZLIB_PATH)/infblock.o $(ZLIB_PATH)/infcodes.o \
                  $(ZLIB_PATH)/inflate.o $(ZLIB_PATH)/inftrees.o \
                  $(ZLIB_PATH)/infutil.o $(ZLIB_PATH)/inffast.o \
                  $(ZLIB_PATH)/trees.o $(ZLIB_PATH)/zutil.o
ENGINE_OBJECTS  = $(ZLIB_OBJECTS) cabinet.o mszip.o raw.o
TEST_OBJECTS    = $(ENGINE_OBJECTS) test.o
OBJECTS         = $(ENGINE_OBJECTS) main.o dfp.o
TARGET          = cabman
PROGS           = $(TARGET).exe test.exe

#FIXME: zlib should be compiled and installed in the SDK by the master makefile
CFLAGS 		+= -O3 -I$(ZLIB_PATH)

CLEAN_FILES = *.o $(TARGET).exe $(TARGET).sym test.exe test.sym

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

#FIXME: zlib should be compiled and installed in the SDK by the master makefile
$(ZLIB_PATH)/zlib.a:
	$(MAKE) -C $(ZLIB_PATH) -f makefile.reactos

$(TARGET).exe: $(OBJECTS) $(ZLIB_PATH)/zlib.a
	$(CC) $(OBJECTS) -lstdc++ -o $(TARGET).exe
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym
	
test.exe: $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -lstdc++ -o test.exe
	$(NM) --numeric-sort test.exe > test.sym

include ../../../rules.mak

