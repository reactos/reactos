#
#
#
OBJECTS= args.o

all: args.exe

.phony: all

clean:
	- $(RM) args.o
	- $(RM) args.exe
	- $(RM) args.sym

.phony: clean

args.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
