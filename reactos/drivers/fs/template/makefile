#
#
#
OBJECTS = template.o

all: tfsd.o

.phony: all

clean:
	- $(RM) template.o
	- $(RM) tfsd.o

.phony: clean

tfsd.o: template.o
	$(LD) -r template.o -o tfsd.o
