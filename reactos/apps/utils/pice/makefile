# $Id: makefile,v 1.2 2003/02/28 22:51:16 gvg Exp $

all:
	$(MAKE) -C loader
	$(MAKE) -C module

implib:
	$(MAKE) -C loader implib
	$(MAKE) -C module implib
	
clean:
	$(MAKE) -C loader clean
	$(MAKE) -C module clean

dist:
	$(MAKE) -C loader dist
	$(MAKE) -C module dist

install:
	$(MAKE) -C loader install
	$(MAKE) -C module install
