#
#
#
PATH_TO_TOP=../..

TARGET_NAME=alive

OBJECTS=\
	../common/crt0.o	\
	$(TARGET_NAME).o

LIBRARIES=\
	$(PATH_TO_TOP)/lib/kernel32/kernel32.a\
	$(PATH_TO_TOP)/lib/crtdll/crtdll.a\
	$(PATH_TO_TOP)/lib/user32/user32.a

PROGS=\
	$(TARGET_NAME).exe

BASE_CFLAGS = -I$(PATH_TO_TOP)/include

all: $(PROGS)

.phony: all

clean:
	- $(RM) $(TARGET_NAME).o
	- $(RM) $(TARGET_NAME).exe
	- $(RM) $(TARGET_NAME).sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=$(PATH_TO_TOP)/$(DIST_DIR)/apps/%)

$(PROGS:%=$(PATH_TO_TOP)/$(DIST_DIR)/apps/%): $(PATH_TO_TOP)/$(DIST_DIR)/apps/%: %
	$(CP) $* $(PATH_TO_TOP)/$(DIST_DIR)/apps/$*

$(TARGET_NAME).exe: $(OBJECTS)
	$(LD)\
		$(OBJECTS)\
		$(LIBRARIES)\
		-o $(TARGET_NAME).exe
	$(NM) --numeric-sort $(TARGET_NAME).exe > $(TARGET_NAME).sym

include $(PATH_TO_TOP)/rules.mak
