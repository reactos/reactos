# $Id: makefile,v 1.13 2001/06/25 12:32:27 ekohl Exp $
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

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
ifeq ($(DOSCLI),yes)
	- $(RM) api\*.o
	- $(RM) sbapi\*.o
else
	- $(RM) api/*.o
	- $(RM) sbapi/*.o
endif
	- $(RM) *.o
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

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
