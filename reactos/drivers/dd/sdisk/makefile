#
#
#
PATH_TO_TOP = ../../..

OBJECTS= sdisk.o ../../../ntoskrnl/ntoskrnl.a

all: sdisk.sys

.phony: all

clean:
	- $(RM) sdisk.o junk.tmp base.tmp temp.exp sdisk.sys

.phony: clean

sdisk.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname sdisk.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o sdisk.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp

WIN32_LEAN_AND_MEAN = yes
include ../../../rules.mak

