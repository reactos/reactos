all: shell.bin

OBJECTS= ../common/crt0.o shell.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

shell.bin: $(OBJECTS) $(LIBS)
	$(LD) -Ttext 0x10000 $(OBJECTS) $(LIBS) -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym
	$(OBJCOPY) -O binary shell.exe shell.bin

include ../../rules.mak
