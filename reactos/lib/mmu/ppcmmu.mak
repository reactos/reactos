O=$(INTERMEDIATE)/lib/ppcmmu
S=lib/ppcmmu
CC=powerpc-unknown-elf-gcc -I$T/include/reactos/ppcmmu
AR=powerpc-unknown-elf-ar
OBJCOPY=powerpc-unknown-elf-objcopy
LDSCRIPT=-Wl,-T,$S/ldscript
PPCMMU_TARGETS=$O/libppcmmu.a $O/libppcmmu_code.a

$O:
	mkdir -p $O

$O/libppcmmu.a: $O/mmuutil.o | $O
	mkdir -p `dirname $@`
	$(AR) cr $@ $O/mmuutil.o

$O/mmuutil.o: $S/mmuutil.c | $O
	$(CC) -Iinclude/reactos/libs -g -c -o $@ $S/mmuutil.c

$O/libppcmmu_code.a: $O/mmuobject.o $O/mmuutil.o $O/mmutest.o | $O
	$(CC) -nostartfiles -nostdlib -o $O/mmuobject -Ttext=0x10000 $(LDSCRIPT) -Wl,-u,mmumain -Wl,-u,data_miss_start -Wl,-u,data_miss_end $O/mmuobject.o $O/mmuutil.o $O/mmutest.o
	$(OBJCOPY) -O binary $O/mmuobject mmucode
	$(OBJCOPY) -I binary -O elf32-powerpc -B powerpc:common mmucode $O/mmucode.o
	mkdir -p `dirname $@`
	$(AR) cr $@ $O/mmucode.o

$O/mmuobject.o: $S/mmuobject.c $S/mmuobject.h | $O
	$(CC) -Iinclude/reactos -Iinclude/reactos/libs -g -c -o $@ $S/mmuobject.c

$O/mmutest.o: $S/mmutest.c $S/mmuobject.h | $O
	$(CC) -Iinclude/reactos/libs -g -c -o $@ $S/mmutest.c

ppcmmu_clean:
	rm -f $O/*.o $O/*.a mmucode $O/mmuobject
