# $Id: makefile,v 1.2 2001/07/26 12:35:52 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=class2

OBJECTS = $(TARGET).o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I$(PATH_TO_TOP)/include

all: $(TARGET).sys $(TARGET).sys.unstripped $(TARGET).a

.phony: all

clean:
	- $(RM) *.o $(TARGET).a $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys


dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys


$(TARGET).a: $(TARGET).def
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)
	$(CC) \
		-nostartfiles -nostdlib -e _DriverEntry@8\
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $^
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $@ \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
		-mdll \
		-o $@.unstripped \
		$^
	- $(RM) temp.exp
	- $(NM) --numeric-sort $@.unstripped > $@.sym
	$(STRIP) --strip-debug $<
	$(CC) \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $^
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $@ \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
		-mdll \
		-o $@ \
		$^
	- $(RM) temp.exp

WARNINGS_ARE_ERRORS = yes
include $(PATH_TO_TOP)/rules.mak
