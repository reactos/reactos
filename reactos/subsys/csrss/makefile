# $Id: makefile,v 1.14 2001/07/15 21:19:02 rex Exp $
#
# CSRSS: Client/server runtime subsystem
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET=csrss

CFLAGS = -D__NTAPP__ -I.

OBJECTS_API = api/process.o api/wapi.o api/conio.o api/handle.o

OBJECTS_SBAPI =

OBJECTS_MISC = $(TARGET).o init.o print.o video.o $(TARGET).coff 

OBJECTS = $(OBJECTS_API) $(OBJECTS_SBAPI) $(OBJECTS_MISC)

CLEAN_FILES = $(OBJECTS) $(TARGET).exe

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) $(CLEAN_FILES)

.phony: clean

$(TARGET).coff: $(TARGET).rc
	$(RC) $(TARGET).rc $(TARGET).coff

$(TARGET).exe: $(OBJECTS) $(LIBS)
	$(LD) \
		$(OBJECTS) \
		$(LIBS) \
		-o $(TARGET).exe \
		--subsystem native
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


install: $(FLOPPY_DIR)/subsys/$(TARGET).exe

$(FLOPPY_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
	$(CP) $(TARGET).exe $(FLOPPY_DIR)/subsys/$(TARGET).exe


dist: $(DIST_DIR)/subsys/$(TARGET).exe

$(DIST_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
	$(CP) $(TARGET).exe ../../$(DIST_DIR)/subsys/$(TARGET).exe


WITH_DEBUGGING = yes
include ../../rules.mak

# EOF
