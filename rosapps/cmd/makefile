all: cmd.bin

OBJECTS = ../common/crt0.o cmd.o

LIBS = ../../lib/mingw32/mingw32.a ../../lib/crtdll/crtdll.a \
       ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

cmd.bin: $(OBJECTS)
	$(LD) -Ttext 0x10000 $(OBJECTS) $(LIBS) -o cmd.exe
	$(OBJCOPY) -O binary cmd.exe cmd.bin

include ../../rules.mak
