all: dummy

OBJECTS = file/file.o file/lfile.o mem/virtual.o mem/local.o mem/global.o

kernel32_lib: $(OBJECTS)
	$(LD) --oformat=coff-go32 -Ttext b0000000 $(LDFLAGS) $(OBJECTS) -o kernel32.dll

dummy:

include ../../rules.mak
