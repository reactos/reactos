# $Id: makefile,v 1.7 1999/12/04 20:58:41 ea Exp $
#
#
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

floppy: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys $(FLOPPY_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys
endif

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys ..\..\..\$(DIST_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys
endif

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
