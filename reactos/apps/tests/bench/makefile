#
#
#
BENCH_PROGS = bench-thread

all: $(BENCH_PROGS:%=%.exe)

.phony: all

clean: $(BENCH_PROGS:%=%_clean)

$(BENCH_PROGS:%=%_clean): %_clean:
	- $(RM) $*.o
	- $(RM) $*.exe
	- $(RM) $*.sym

.phony: clean

floppy: # $(BENCH_PROGS:%=%_floppy)

$(BENCH_PROGS:%=%_floppy): %_floppy: $(FLOPPY_DIR)/apps/%.exe

$(BENCH_PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
ifeq ($(DOSCLI),yes)
	$(CP) $*.exe $(FLOPPY_DIR)\apps\$*.exe
else
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe
endif

bench-thread.exe: bench-thread.c
	$(CC) bench-thread.c -lkernel32 -o bench-thread.exe
	$(NM) --numeric-sort bench-thread.exe > bench-thread.sym

include ../../rules.mak
