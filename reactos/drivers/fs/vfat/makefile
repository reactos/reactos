%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM) $(NFLAGS) $< -o $@

OBJECTS= blockdev.o iface.o dir.o dirwr.o

all: vfatfsd.o
tests: tstvfat5.bin tstvfat6.bin

vfatfsd.o: $(OBJECTS)
	$(LD) $(OBJECTS) -r -o vfatfsd.o 
	$(NM) --numeric-sort vfatfsd.o > vfatfsd.sym

OBJECTS2 = ../../../apps/common/crt0.o
LIBS= ../../../lib/kernel32/kernel32.a ../../../lib/ntdll/ntdll.a

%.bin: $(OBJECTS) %.o
	$(LD) -Ttext 0x10000 $(OBJECTS2) $*.o $(LIBS) -o $*.exe
	$(OBJCOPY) -O binary $*.exe $*.bin

include ../../../rules.mak
