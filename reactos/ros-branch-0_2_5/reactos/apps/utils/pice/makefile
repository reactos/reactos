# $Id: makefile,v 1.3 2003/11/24 16:47:44 ekohl Exp $

all:
	$(MAKE) -C loader
	$(MAKE) -C module

implib:
	$(MAKE) -C loader implib
	$(MAKE) -C module implib
	
clean:
	$(MAKE) -C loader clean
	$(MAKE) -C module clean

install:
	$(MAKE) -C loader install
	$(MAKE) -C module install
