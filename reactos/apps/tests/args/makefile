all: args.bin

OBJECTS= ../common/crt0.o args.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

args.bin: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs -Ttext 0x10000 $(OBJECTS) $(LIBS) -lgcc \
	      -o args.exe
	$(NM) --numeric-sort args.exe > args.sym
	../../ntoskrnl/utils/pe2bin/pe2bin$(EXE_POSTFIX) 0x10000 args.exe \
	                                                         args.bin
