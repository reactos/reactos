all: test.bin

test.bin: test.o
	$(LD) -Ttext 0x10000 test.o ../../lib/ntdll/ntdll.a -o test.exe
	$(OBJCOPY) -O binary test.exe test.bin

include ../../rules.mak
