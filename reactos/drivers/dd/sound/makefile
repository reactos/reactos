#
#
#
PATH_TO_TOP = ../../..

OBJECTS= sound.o ../../../ntoskrnl/ntoskrnl.a

all: sound.sys

.phony: all

clean:
	- $(RM) sound.o junk.tmp base.tmp temp.exp sound.sys

.phony: clean

sound.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname sound.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o sound.sys $(OBJECTS) -Wl,temp.exp

sound.o: sound.c dsp.c mixer.c wave.h wave.c


