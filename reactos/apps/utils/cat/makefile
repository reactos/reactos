#
#
#
OBJECTS= cat.o

all: cat.exe

.phony: all

clean:
	- $(RM) cat.o
	- $(RM) cat.exe
	- $(RM) cat.sym

.phony: clean

cat.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o cat.exe
	$(NM) --numeric-sort cat.exe > args.sym

include ../../rules.mak
