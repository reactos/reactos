#
#
#
OBJECTS = keyboard.o ../../../ntoskrnl/ntoskrnl.a

all: keyboard.sys

.phony: all

clean:
	- $(RM) keyboard.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) keyboard.sys

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/keyboard.sys

$(FLOPPY_DIR)/drivers/keyboard.sys: keyboard.sys
ifeq ($(DOSCLI),yes)
	$(CP) keyboard.sys $(FLOPPY_DIR)\drivers\keyboard.sys
else
	$(CP) keyboard.sys $(FLOPPY_DIR)/drviers/keyboard.sys
endif

keyboard.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname keyboard.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
              -specs=../../svc_specs -mdll -o keyboard.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp
