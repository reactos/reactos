all: shell.bin

OBJECTS = ../common/crt0.o shell.o

shell.bin: $(OBJECTS)
	$(LD) -Ttext 0x10000 $(OBJECTS) ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a -o shell.exe
	$(OBJCOPY) -O binary shell.exe shell.bin

include ../../rules.mak
