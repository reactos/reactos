#
#
#

all: loadros.com

loadros.com: loadros.asm
	nasm -fbin loadros.asm -o loadros.com

clean:
	- $(RM) loadros.com

.PHONY: clean

include ../../rules.mak
