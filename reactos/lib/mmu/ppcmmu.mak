O=obj-ppc/lib/ppcmmu
S=lib/ppcmmu
CC=powerpc-unknown-linux-gnu-gcc -I$T/include/reactos/ppcmmu
AR=powerpc-unknown-linux-gnu-ar
OBJCOPY=powerpc-unknown-linux-gnu-objcopy
LDSCRIPT=-Wl,-T,$S/ldscript
PPCMMU_TARGETS=$O/libppcmmu.a $O/libppcmmu_code.a


$O/libppcmmu.a: $O/mmuutil.o
	mkdir -p `dirname $@`
	$(AR) cr $@ $O/mmuutil.o

$O/mmuutil.o: $S/mmuutil.c
	$(CC) -g -c $S/mmuutil.c

$O/libppcmmu_code.a: $O/mmuobject.o $O/mmuutil.o $O/mmutest.o
	$(CC) -g -nostartfiles -nostdlib -o $O/mmuobject -Ttext=0x10000 $(LDSCRIPT) -Wl,-u,mmumain -Wl,-u,data_miss_start -Wl,-u,data_miss_end $O/mmuobject.o $O/mmuutil.o $O/mmutest.o
	$(OBJCOPY) -O binary $O/mmuobject $O/mmucode
	$(OBJCOPY) -I binary -O elf32-powerpc -B powerpc:common $O/mmucode $O/mmucode.o
	mkdir -p `dirname $@`
	$(AR) cr $@ $O/mmucode.o

$O/mmuobject.o: $S/mmuobject.c $S/mmuobject.h
	$(CC) -Iinclude/reactos -g -c $S/mmuobject.c

$O/mmutest.o: $S/mmutest.c $S/mmuobject.h
	$(CC) -g -c $S/mmutest.c

clean:
	rm -f $O/*.o $O/*.a $O/mmucode $O/mmuobject
