# $Id: Makefile,v 1.1 2000/08/27 16:35:04 chorns Exp $
#
# NE2000.SYS build spec
#

TARGETNAME=ne2000

BASE_CFLAGS = -I./include -I../../../../include -DNDIS_MINIPORT_DRIVER

RESOURCE_OBJECT  = $(TARGETNAME).coff
NE2000_OBJECTS   = ne2000/main.o ne2000/8390.o
IMPORT_LIBS      = ../../ndis/ndis.a ../../../../ntoskrnl/ntoskrnl.a


all: objects $(TARGETNAME).sys

objects:
	mkdir objects

objects/ne2000.o: $(NE2000_OBJECTS)
	$(LD) -r $(NE2000_OBJECTS) -o objects/ne2000.o

OBJECTS = objects/ne2000.o

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = \
		*.o ne2000\*.o objects\*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = \
		*.o ne2000\*.o objects/*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
endif


$(TARGETNAME).sys: $(OBJECTS) $(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def $(TARGETNAME).def \
		--output-lib $(TARGETNAME).a \
		--kill-at
	$(CC) \
        -specs=../../../../specs \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DriverEntry@8 \
		-o junk.tmp \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
        $(TARGETNAME).o \
        $(IMPORT_LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).def \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
        -specs=../../../../specs \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DriverEntry@8 \
		-o $(TARGETNAME).sys \
		-Wl,--image-base,0x0 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,temp.exp \
		$(TARGETNAME).o \
        $(IMPORT_LIBS)
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).sys > $(TARGETNAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)\drivers\$(TARGETNAME).sys
else
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys
endif

dist: $(DIST_DIR)/drivers/$(TARGETNAME).sys

$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).sys ..\..\$(DIST_DIR)\drivers\$(TARGETNAME).sys
else
	$(CP) $(TARGETNAME).sys ../../$(DIST_DIR)/drivers/$(TARGETNAME).sys
endif

#WITH_DEBUGGING      = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../../../rules.mak
