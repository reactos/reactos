#
#
#
TARGET=vidport
OBJECTS = vidport.o ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I../../../include

all: vidport.sys vidport.a

.phony: all

clean:
	- $(RM) vidport.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) vidport.sys

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

vidport.a: vidport.sys vidport.def
	$(DLLTOOL) \
		--dllname vidport.sys \
		--def vidport.def \
		--output-lib vidport.a


include ../../../rules.mak
