#
#
#
OBJECTS = template.o

BASE_CFLAGS = -I../../../include

all: tfsd.o

.phony: all

clean:
	- $(RM) template.o
	- $(RM) tfsd.o

.phony: clean

tfsd.o: template.o
	$(LD) -r template.o -o tfsd.o

include ../../rules.mak
