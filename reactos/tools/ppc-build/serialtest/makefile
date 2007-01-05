all: serialtest.exe serial.dll

libserial.a serial.exp:
	reactos-powerpc-dlltool -D serial.dll -e serial.exp -l libserial.a -d serial.def -k -A -S reactos-powerpc-as -L reactos-powerpc-ld

serialtest.exe: serial.h serialtest.c libserial.a
	reactos-powerpc-gcc -Wl,--image-base=0x400000 -Wl,--entry,main -nostdlib -nostartfiles -o $@ serialtest.c -L. -lserial

serial.dll: serial.o serial.exp
	reactos-powerpc-gcc -Wl,--image-base=0x600000 -Wl,--entry,sync -nostdlib -nostartfiles -shared -o $@ serial.o serial.exp

serial.o: serial.c
	reactos-powerpc-gcc -c $<

clean:
	rm -rf *.o *.a *.exp *.dll
