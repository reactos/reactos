%.o: %.cpp
	g++ -g -c -o $@ $<

all: alink.exe

clean:
	rm -rf alink.exe *.o

alink.o combine.o util.o output.o objload.o coff.o cofflib.o elf.o: alink.h

alink.exe: alink.o combine.o util.o output.o objload.o coff.o cofflib.o elf.o
	g++ -g -o $@ $^
