all: hello.exe

OBJECTS = ../common/crt0.o hello.o
LIBS = ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

hello.exe: $(OBJECTS) $(LIBS)
	$(LD) $(OBJECTS) $(LIBS) -o hello.exe

include ../../rules.mak
