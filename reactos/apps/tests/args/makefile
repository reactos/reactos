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

floppy: $(FLOPPY_DIR)/apps/args.exe

$(FLOPPY_DIR)/apps/args.exe: args.exe
ifeq ($(DOSCLI),yes)
	$(CP) args.exe $(FLOPPY_DIR)\apps\args.exe
else
	$(CP) args.exe $(FLOPPY_DIR)/apps\args.exe
endif

args.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
