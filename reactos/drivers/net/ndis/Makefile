# NDIS.SYS - NDIS library
#
PATH_TO_TOP = ../../..

TARGETNAME=ndis

CFLAGS = -I./include -DNDIS_WRAPPER -DBINARY_COMPATIBLE=1

RESOURCE_OBJECT = $(TARGETNAME).coff
NDIS_OBJECTS    = ndis/main.o ndis/40gone.o ndis/50gone.o ndis/buffer.o ndis/cl.o \
                  ndis/cm.o ndis/co.o ndis/control.o ndis/hardware.o \
                  ndis/io.o ndis/memory.o ndis/miniport.o \
		  ndis/protocol.o ndis/string.o ndis/stubs.o ndis/time.o

all: objects $(TARGETNAME).sys

objects:
	mkdir objects

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

OBJECTS = $(NDIS_OBJECTS) $(RESOURCE_OBJECT) ../../../ntoskrnl/ntoskrnl.a


ifeq ($(DOSCLI),yes)
CLEAN_FILES = *.o objects\*.o ndis\*.o $(TARGETNAME).coff $(TARGETNAME).o $(TARGETNAME).a \
              junk.tmp base.tmp temp.exp $(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = *.o objects/*.o ndis/*.o $(TARGETNAME).coff $(TARGETNAME).o $(TARGETNAME).a \
              junk.tmp base.tmp temp.exp $(TARGETNAME).sys $(TARGETNAME).sym
endif

$(TARGETNAME).sys: $(OBJECTS)
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,-e,_DriverEntry@8 \
		-Wl,--base-file,base.tmp \
		-Wl,--defsym,_end=end  \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
        $(OBJECTS) \
        -o junk.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
        --dllname $(TARGETNAME).sys \
        --base-file base.tmp \
        --output-exp temp.exp \
        --def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
        $(OBJECTS) \
        -o $(TARGETNAME).sys
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).sys > $(TARGETNAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: $(DIST_DIR)/drivers/$(TARGETNAME).sys

$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys

#WITH_DEBUGGING      = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
