all: args.exe

OBJECTS= ../common/crt0.o args.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

args.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
