# $Id: makefile,v 1.12 2003/04/05 09:37:42 chorns Exp $
#
#
all:
	$(MAKE) -C display
	$(MAKE) -C miniport

implib:
	$(MAKE) -C display implib
	$(MAKE) -C miniport implib
	
clean:
	$(MAKE) -C display clean
	$(MAKE) -C miniport clean

dist:
	$(MAKE) -C display dist
	$(MAKE) -C miniport dist

install:
	$(MAKE) -C display install
	$(MAKE) -C miniport install

bootcd:
	$(MAKE) -C display bootcd
	$(MAKE) -C miniport bootcd
