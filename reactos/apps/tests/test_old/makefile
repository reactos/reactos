#
#
#
PATH_TO_TOP = ../..

PROGS= test-stdio tst-printf tstdiomisc bug2 bug3 \
       temptest test-fseek test_rdwr testfsd time_t

all: $(PROGS:%=%.exe)

.phony: all

implib:

clean:
	- $(RM) *.o *.exe *.sym

.phony: implib clean

install: # $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe

dist: $(PROGS:%=$(DIST_DIR)/apps/%.exe)

$(PROGS:%=$(DIST_DIR)/apps/%.exe): $(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(DIST_DIR)/apps/$*.exe

bug2.exe: bug2.c
	$(CC) bug2.c -lkernel32 -o bug2.exe
	$(NM) --numeric-sort bug2.exe > bug2.sym

bug3.exe: bug3.c
	$(CC) bug3.c -lkernel32 -o bug3.exe
	$(NM) --numeric-sort bug3.exe > bug3.sym

temptest.exe: temptest.c
	$(CC) temptest.c -lkernel32 -o temptest.exe
	$(NM) --numeric-sort temptest.exe > temptest.sym

test-fseek.exe: test-fseek.c
	$(CC) test-fseek.c -lkernel32 -o test-fseek.exe
	$(NM) --numeric-sort test-fseek.exe > test-fseek.sym

test-fwrite.exe: test-fwrite.c
	$(CC) test-fwrite.c -lkernel32 -o test-fwrite.exe
	$(NM) --numeric-sort test-fwrite.exe > test-fwrite.sym

test_rdwr.exe: test_rdwr.c
	$(CC) test_rdwr.c -lkernel32 -o test_rdwr.exe
	$(NM) --numeric-sort test_rdwr.exe > test_rdwr.sym

test-stdio.exe: test-stdio.c
	$(CC) test-stdio.c -lkernel32 -o test-stdio.exe
	$(NM) --numeric-sort test-stdio.exe > test-stdio.sym

tst-printf.exe: tst-printf.c
	$(CC) tst-printf.c -lkernel32 -o tst-printf.exe
	$(NM) --numeric-sort tst-printf.exe > tst-printf.sym

tstdiomisc.exe: tstdiomisc.c
	$(CC) tstdiomisc.c -lkernel32 -o tstdiomisc.exe
	$(NM) --numeric-sort tstdiomisc.exe > tstdiomisc.sym

testfsd.exe: testfsd.c testsuite.c
	$(CC) testfsd.c testsuite.c -lkernel32 -o testfsd.exe
	$(NM) --numeric-sort testfsd.exe > testfsd.sym

time_t.exe: time_t.c
	$(CC) time_t.c -lkernel32 -o time_t.exe
	$(NM) --numeric-sort time_t.exe > time_t.sym

include ../../rules.mak
