all: shell.bin

OBJECTS= ../common/crt0.o shell.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

shell.bin: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs -Ttext 0x10000 $(OBJECTS) $(LIBS) -lgcc \
	      -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym
	../../ntoskrnl/utils/pe2bin/pe2bin$(EXE_POSTFIX) 0x10000 shell.exe \
	                                                         shell.bin

include ../../rules.mak
