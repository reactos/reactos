#
#
#
OBJECTS= ../common/crt0.o args.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a

all: args.exe

.phony: all

clean:
	- $(RM) args.o
	- $(RM) args.exe
	- $(RM) args.sym

.phony: clean

args.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
