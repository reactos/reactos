all: hello.bin

OBJECTS = ../common/crt0.o hello.o

hello.bin: $(OBJECTS)
	$(LD) -Ttext 0x10000 $(OBJECTS) ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a -o hello.exe
	$(OBJCOPY) -O binary hello.exe hello.bin

include ../../rules.mak
