#
#
#
OBJECTS= ../common/crt0.o shell.o
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a
CLEAN_FILES= shell.o shell.exe shell.sym

all: shell.exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

shell.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym

include ../../rules.mak
