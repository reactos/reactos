#
#
#
OBJECTS= parallel.o ../../../ntoskrnl/ntoskrnl.a

all: parallel.sys

.phony: all

clean:
	- $(RM) parallel.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) parallel.sys

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/parallel.sys

$(FLOPPY_DIR)/drivers/parallel.sys: parallel.sys
ifeq ($(DOSCLI),yes)
	$(CP) parallel.sys $(FLOPPY_DIR)\drivers\parallel.sys
else
	$(CP) parallel.sys $(FLOPPY_DIR)/drviers/parallel.sys
endif

parallel.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname parallel.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o parallel.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp


