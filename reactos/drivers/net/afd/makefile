# AFD.SYS - Ancillary Function Driver

PATH_TO_TOP = ../../..

TARGETNAME=afd

CFLAGS = -D__NTDRIVER__ -I./include -DDBG

RESOURCE_OBJECT = $(TARGETNAME).coff
AFD_OBJECTS = afd/afd.o afd/dispatch.o afd/event.o afd/opnclose.o afd/rdwr.o \
              afd/routines.o afd/tdi.o

all: $(TARGETNAME).sys

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

OBJECTS = $(AFD_OBJECTS) $(RESOURCE_OBJECT) ../../../ntoskrnl/ntoskrnl.a


CLEAN_FILES = afd/*.o $(TARGETNAME).coff $ \
              junk.tmp base.tmp temp.exp $(TARGETNAME).sys $(TARGETNAME).sym

$(TARGETNAME).sys: $(OBJECTS)
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
        --output-exp temp.exp
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

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

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
	$(CP) $(TARGETNAME).sys ..\..\..\$(DIST_DIR)\drivers\$(TARGETNAME).sys
else
	$(CP) $(TARGETNAME).sys ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys
endif

#WITH_DEBUGGING      = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
