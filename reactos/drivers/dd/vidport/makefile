# $Id: makefile,v 1.10 2000/11/20 19:59:11 ekohl Exp $
#
#
PATH_TO_TOP = ../../..
TARGET=vidport
OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I../../../include

all: $(TARGET).sys vidport.a

.phony: all

clean:
	- $(RM) $(TARGET).o
	- $(RM) $(TARGET).a
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys: $(OBJECTS)
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) \
                $(OBJECTS) \
                -Wl,-d -specs=../../svc_specs \
                -mdll \
                -o junk.tmp \
                -Wl,--image-base,0x10000 \
                -Wl,--defsym,_end=end \
	        -Wl,--defsym,_edata=__data_end__ \
                -Wl,--defsym,_etext=etext \
	        -Wl,--base-file,base.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
                --dllname $(TARGET).sys \
                --base-file base.tmp \
	        --output-exp temp.exp \
                --def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
                $(OBJECTS) \
                -specs=../../svc_specs \
                -mdll \
                -o $(TARGET).sys \
                -Wl,--image-base,0x10000 \
                -Wl,-e,_DriverEntry@8 \
                -Wl,--defsym,_end=end \
	        -Wl,--defsym,_edata=__data_end__ \
                -Wl,--defsym,_etext=etext \
                -Wl,temp.exp 
	- $(RM) temp.exp

include ../../../rules.mak
