O=$(INTERMEDIATE)/lib/ppcmmu
S=lib/ppcmmu
CC=powerpc-unknown-elf-gcc -I$T/include/reactos/ppcmmu
AR=powerpc-unknown-elf-ar
CFLAGS=-Iinclude/reactos/libs -Iinclude/crt -Iinclude/reactos -D__cdecl__=
OBJCOPY=powerpc-unknown-elf-objcopy
LDSCRIPT=-Wl,-T,$S/ldscript
PPCMMU_TARGETS=$O/libppcmmu_code.a
MMUOBJECT_OBJS=$O/devint.o $O/mmuobject.o $O/mmuutil_object.o $O/mmutest.o $O/gdblib.o

mkdir_ppcmmu:
	-mkdir -p $O

$O/mmuutil_object.o: $S/mmuutil.c | mkdir_ppcmmu
	$(CC) $(CFLAGS) -g -c -o $@ $S/mmuutil.c

$O/libppcmmu_code.a: $(MMUOBJECT_OBJS)
	$(CC) -Wl,-N -nostartfiles -nostdlib -o $O/mmuobject -Ttext=0x10000 $(LDSCRIPT) -Wl,-u,mmumain -Wl,-u,data_miss_start -Wl,-u,data_miss_end $(MMUOBJECT_OBJS)
	$(OBJCOPY) -O binary $O/mmuobject mmucode
	$(OBJCOPY) -I binary -O elf32-powerpc -B powerpc:common mmucode $O/mmucode.o
	mkdir -p `dirname $@`
	$(AR) cr $@ $O/mmucode.o

$O/gdblib.o: $S/gdblib.c | mkdir_ppcmmu
	$(CC) $(CFLAGS) -g -c -o $@ $S/gdblib.c

$O/devint.o: $S/devint.s | mkdir_ppcmmu
	$(CC) $(CFLAGS) -g -c -o $@ $S/devint.s

$O/mmuobject.o: $S/mmuobject.c $S/mmuobject.h | mkdir_ppcmmu
	$(CC) $(CFLAGS) -g -c -o $@ $S/mmuobject.c

$O/mmutest.o: $S/mmutest.c $S/mmuobject.h | mkdir_ppcmmu
	$(CC) $(CFLAGS) -g -c -o $@ $S/mmutest.c

ppcmmuobject_clean:
	rm -f $O/*.o $O/*.a mmucode $O/mmuobject
