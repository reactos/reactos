T=../..
O=$T/obj-ppc/lib/mmu
CC=powerpc-unknown-linux-gnu-gcc -I$T/include/reactos/ppcmmu
AR=powerpc-unknown-linux-gnu-ar
OBJCOPY=powerpc-unknown-linux-gnu-objcopy

all: $O/libmmu.a $O/libmmu_code.a


$O/libmmu.a: mmuutil.o
	mkdir -p `dirname $@`
	$(AR) cq $@ mmuutil.o

mmuutil.o: mmuutil.c
	$(CC) -g -c mmuutil.c

$O/libmmu_code.a: mmuobject.o mmuutil.o mmutest.o
	$(CC) -g -nostartfiles -nostdlib -o mmuobject -Ttext=0x10000 mmuobject.o mmuutil.o mmutest.o
	$(OBJCOPY) -O binary mmuobject mmucode
	$(OBJCOPY) -I binary -O elf32-powerpc -B powerpc:common mmucode mmucode.o
	mkdir -p `dirname $@`
	$(AR) cq $@ mmucode.o

mmuobject.o: mmuobject.c mmuobject.h
	$(CC) -g -c mmuobject.c

mmutest.o: mmutest.c mmuobject.h
	$(CC) -g -c mmutest.c

clean:
	rm *.o $O/*.a
