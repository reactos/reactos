# TDITEST.SYS - TDI test driver
#
PATH_TO_TOP = ../../..

TARGETNAME=tditest

CFLAGS = -D__NTDRIVER__ -I./include 

RESOURCE_OBJECT = $(TARGETNAME).coff
TDITEST_OBJECTS = tditest/tditest.o

all: objects $(TARGETNAME).sys

objects:
	mkdir objects

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

OBJECTS = $(TDITEST_OBJECTS) $(RESOURCE_OBJECT) ../../../ntoskrnl/ntoskrnl.a


CLEAN_FILES = *.o objects/*.o tditest/*.o $(TARGETNAME).coff $(TARGETNAME).o \
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
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: $(DIST_DIR)/drivers/$(TARGETNAME).sys

$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys

include $(PATH_TO_TOP)/rules.mak
