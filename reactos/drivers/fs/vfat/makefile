%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM) $(NFLAGS) $< -o $@

OBJECTS= blockdev.o iface.o dir.o

all: $(OBJECTS)
	$(LD) $(OBJECTS) -r -o vfatfsd.o 
	$(NM) --numeric-sort vfatfsd.o > vfatfsd.sym

include ../../../rules.mak
