#
#
#
OBJECTS= null.o ../../../ntoskrnl/ntoskrnl.a

all: null.sys

.phony: all

clean:
	- $(RM) null.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) null.sys

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/null.sys

$(FLOPPY_DIR)/drivers/null.sys: null.sys
ifeq ($(DOSCLI),yes)
	$(CP) null.sys $(FLOPPY_DIR)\drivers\null.sys
else
	$(CP) null.sys $(FLOPPY_DIR)/drviers/null.sys
endif

null.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname null.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o null.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp


