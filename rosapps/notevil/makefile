# $Id: makefile,v 1.5 2001/10/16 21:02:02 ea Exp $
#
# ReactOS makefile for notevil
# Compiler: egcs 1.1.2
#
PATH_TO_TOP = ..
PATH_TO_REACTOS = $(PATH_TO_TOP)/../reactos

TARGET=notevil

BASE_CFLAGS = -I$(PATH_TO_REACTOS)/include

all: $(TARGET).exe

OBJECTS = $(TARGET).o $(TARGET).coff

CLEAN_FILES = *.o $(TARGET).exe $(TARGET).sym $(TARGET).coff

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

$(TARGET).exe: $(OBJECTS)
	$(CC) $(OBJECTS) \
		-o $(TARGET).exe \
		-lkernel32 \
		-luser32 \
		-lmsvcrt
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym

$(TARGET).coff: $(TARGET).rc
	$(RC) $(RFLAGS) $(TARGET).rc $(TARGET).coff

$(TARGET).o: $(TARGET).c resource.h

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
	$(CP) $* ..\$(DIST_DIR)\apps\$*
else
	$(CP) $* $(PATH_TO_TOP)/$(DIST_DIR)/apps/$*
endif

include $(PATH_TO_TOP)/rules.mak

# EOF
