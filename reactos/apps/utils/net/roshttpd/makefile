#
# ReactOS HTTP Daemon
#
PATH_TO_TOP = ../../..

TARGETNAME=roshttpd

CFLAGS = -Iinclude -DUNICODE -D_UNICODE -DDBG

MAIN_OBJECTS = $(TARGETNAME).o config.o error.o http.o httpd.o
COMMON_OBJECTS = common/list.o common/socket.o common/thread.o common/$(TARGETNAME).coff

OBJECTS = $(MAIN_OBJECTS) $(COMMON_OBJECTS)
PROGS = $(TARGETNAME).exe
LIBS = ../../../lib/kernel32/kernel32.a \
       ../../../lib/ws2_32/ws2_32.a \
	../../../lib/user32/user32.a

ifeq ($(DOSCLI), yes)
CLEAN_FILES = *.o $(TARGETNAME).exe $(TARGETNAME).sym common\*.o
else
CLEAN_FILES = *.o $(TARGETNAME).exe $(TARGETNAME).sym common/*.o
endif

all: $(TARGETNAME).exe

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

$(TARGETNAME).exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGETNAME).exe

include ../../../rules.mak
