#
#
#

OBJECTS = blue.o ../../../ntoskrnl/ntoskrnl.a

all: blue.sys

.phony: all

clean:
	- $(RM) blue.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) blue.sys

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/blue.sys

$(FLOPPY_DIR)/drivers/blue.sys: blue.sys
ifeq ($(DOSCLI),yes)
	$(CP) blue.sys $(FLOPPY_DIR)\drivers\blue.sys
else
	$(CP) blue.sys $(FLOPPY_DIR)/drviers/blue.sys
endif

blue.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname blue.sys --base-file base.tmp \
		   --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o blue.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp


