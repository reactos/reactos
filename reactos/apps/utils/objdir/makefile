#
#
#
TARGET_NAME=objdir
OBJECTS= $(TARGET_NAME).o


PROGS= $(TARGET_NAME).exe 

BASE_CFLAGS = -I../../include -g
LIB_BASE=../../lib
LIBS = \
	$(LIB_BASE)/ntdll/ntdll.a \
	$(LIB_BASE)/kernel32/kernel32.a \
	$(LIB_BASE)/crtdll/crtdll.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) $(TARGET_NAME).o
	- $(RM) $(TARGET_NAME).exe
	- $(RM) $(TARGET_NAME).sym

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

$(TARGET_NAME).exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET_NAME).exe
	$(NM) --numeric-sort $(TARGET_NAME).exe > $(TARGET_NAME).sym


include ../../rules.mak
