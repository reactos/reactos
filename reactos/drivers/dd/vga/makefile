#
#
#
OBJECTS = vga.o ../../../ntoskrnl/ntoskrnl.a ../vidport/vidport.a

all: vga.sys

.phony: all

clean:
	- $(RM) vga.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) vga.sys

.phony: clean

vga.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname vga.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
              -specs=../../svc_specs -mdll -o vga.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp

