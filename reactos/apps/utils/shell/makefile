all: shell.exe

OBJECTS= ../common/crt0.o shell.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

shell.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym

include ../../rules.mak
