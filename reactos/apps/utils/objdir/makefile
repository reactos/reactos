#
#
#
PATH_TO_TOP = ../..

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
	- $(RM) $(TARGET_NAME).o $(TARGET_NAME).exe $(TARGET_NAME).sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

$(TARGET_NAME).exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET_NAME).exe
	$(NM) --numeric-sort $(TARGET_NAME).exe > $(TARGET_NAME).sym


include ../../rules.mak
