#
#
#
TARGET=ide
OBJECTS = ide.o ../../../ntoskrnl/ntoskrnl.a

all: ide.sys

.phony: all

clean:
	- $(RM) ide.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) ide.sys

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

ide.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname ide.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
              -specs=../../svc_specs -mdll -o ide.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp

