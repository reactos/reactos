# $Id: makefile,v 1.1 2002/02/10 02:01:31 chorns Exp $

all:
	make -C loader
	make -C module

implib:
	make -C loader implib
	make -C module implib
	
clean:
	make -C loader clean
	make -C module clean

dist:
	make -C loader dist
	make -C module dist

install:
	make -C loader install
	make -C module install
