T=../..
O=$T/obj-ppc/lib/mmu
CC=powerpc-unknown-linux-gnu-gcc -I$T/include/reactos/ppcmmu
AR=powerpc-unknown-linux-gnu-ar
OBJCOPY=powerpc-unknown-linux-gnu-objcopy
LDSCRIPT=-Wl,-T,ldscript
#LDSCRIPT=

all: $O/libmmu.a $O/libmmu_code.a


$O/libmmu.a: mmuutil.o
	mkdir -p `dirname $@`
	$(AR) cr $@ mmuutil.o

mmuutil.o: mmuutil.c
	$(CC) -g -c mmuutil.c

$O/libmmu_code.a: mmuobject.o mmuutil.o mmutest.o
	$(CC) -g -nostartfiles -nostdlib -o mmuobject -Ttext=0x10000 $(LDSCRIPT) -Wl,-u,mmumain -Wl,-u,data_miss_start -Wl,-u,data_miss_end mmuobject.o mmuutil.o mmutest.o
	$(OBJCOPY) -O binary mmuobject mmucode
	$(OBJCOPY) -I binary -O elf32-powerpc -B powerpc:common mmucode mmucode.o
	mkdir -p `dirname $@`
	$(AR) cr $@ mmucode.o

mmuobject.o: mmuobject.c mmuobject.h
	$(CC) -I../../include/reactos -g -c mmuobject.c

mmutest.o: mmutest.c mmuobject.h
	$(CC) -g -c mmutest.c

clean:
	rm -f *.o $O/*.a mmucode mmuobject
