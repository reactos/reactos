#
#
#
OBJECTS= mouse.o ../../../ntoskrnl/ntoskrnl.a

all: mouse.o

.phony: all

clean:
	- $(RM) mouse.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) mouse.sys

.phony: clean

floppy: # $(FLOPPY_DIR)/drivers/mouse.sys

$(FLOPPY_DIR)/drivers/mouse.sys: mouse.sys
ifeq ($(DOSCLI),yes)
	$(CP) mouse.sys $(FLOPPY_DIR)\drivers\mouse.sys
else
	$(CP) mouse.sys $(FLOPPY_DIR)/drviers/mouse.sys
endif

mouse.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname mouse.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o mouse.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp


