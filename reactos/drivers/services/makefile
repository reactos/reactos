# $Id: makefile,v 1.1 2000/03/26 22:00:09 dwelch Exp $
#
# CSRSS: Client/server runtime subsystem
#
# ReactOS Operating System
#
TARGET = services

BASE_CFLAGS = -I../../include -I.

OBJECTS = 

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
