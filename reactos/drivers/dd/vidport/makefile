#
#
#
OBJECTS = vidport.o ../../../ntoskrnl/ntoskrnl.a

all: vidport.sys

.phony: all

clean:
	- $(RM) vidport.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) vidport.sys

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/vidport.sys

$(FLOPPY_DIR)/drivers/vidport.sys: vidport.sys
ifeq ($(DOSCLI),yes)
	$(CP) vidport.sys $(FLOPPY_DIR)\drivers\vidport.sys
else
	$(CP) vidport.sys $(FLOPPY_DIR)/drviers/vidport.sys
endif

vidport.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname vidport.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
              -specs=../../svc_specs -mdll -o vidport.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp

