#
#
#
OBJECTS= test-stdio.o

all: test-stdio.exe

.phony: all

clean:
	- $(RM) test-stdio.o
	- $(RM) test-stdio.exe
	- $(RM) test-stdio.sym

.phony: clean

test-stdio.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -lkernel32 -o test-stdio.exe
	$(NM) --numeric-sort test-stdio.exe > test-stdio.sym

include ../../rules.mak
