%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM) $(NFLAGS) $< -o $@

all: blockdev.o iface.o
	$(LD) iface.o blockdev.o -r -o vfatfsd.o 

include ../../../rules.mak
