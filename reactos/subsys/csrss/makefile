# $Id: makefile,v 1.5 2000/02/27 02:11:54 ekohl Exp $
#
# CSRSS: Client/server runtime subsystem
#
# ReactOS Operating System
#
TARGET=csrss

BASE_CFLAGS = -I../../include -I.

OBJECTS_API = api/process.o api/wapi.o api/conio.o

OBJECTS_SBAPI =

OBJECTS_MISC = $(TARGET).o init.o print.o $(TARGET).coff 

OBJECTS = $(OBJECTS_API) $(OBJECTS_SBAPI) $(OBJECTS_MISC)

LIBS = ../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) api/*.o
	- $(RM) sbapi/*.o
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


floppy: $(FLOPPY_DIR)/subsys/$(TARGET).exe

$(FLOPPY_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).exe $(FLOPPY_DIR)\subsys\$(TARGET).exe
else
	$(CP) $(TARGET).exe $(FLOPPY_DIR)/subsys/$(TARGET).exe
endif


dist: $(DIST_DIR)/subsys/$(TARGET).exe

$(DIST_DIR)/subsys/$(TARGET).exe: $(TARGET).exe
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).exe ..\..\$(DIST_DIR)\subsys\$(TARGET).exe
else
	$(CP) $(TARGET).exe ../../$(DIST_DIR)/subsys/$(TARGET).exe
endif


include ../../rules.mak

# EOF
