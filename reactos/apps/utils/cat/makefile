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

floppy: $(FLOPPY_DIR)/apps/cat.exe

$(FLOPPY_DIR)/apps/cat.exe: cat.exe
ifeq ($(DOSCLI),yes)
	$(CP) cat.exe $(FLOPPY_DIR)\apps\cat.exe
else
	$(CP) cat.exe $(FLOPPY_DIR)/apps/cat.exe
endif

cat.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o cat.exe
	$(NM) --numeric-sort cat.exe > args.sym

include ../../rules.mak
